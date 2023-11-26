#define STB_IMAGE_IMPLEMENTATION
#include "renderer.h"
#include "game.h"
#include "memory.h"
#include "gpu.h"
#include "system.h"
#include "chunk.h"
#include <stb_image.h>

#define MAX_SOLID2D_VERTICES 1000
#define MAX_SOLID2D_INDICES  1500
#define MAX_SPRITE_COMMANDS 4096
#define CHUNK_DATA_ARENA_SIZE WT_MEGABYTES(32)

typedef struct
{
  wt_vec2f_t pos;
  wt_color_t color;
} solid2d_vertex_t;

typedef struct
{
  wt_rect_t src;
  wt_rect_t dst;
  f32 rotation;
} sprite2d_command_t;

typedef struct
{
  wt_vec2f_t rc_atlas_size;
} sprite2d_cbuffer_t;

typedef enum
{
  BATCH2D_SOLID,
  BATCH2D_SPRITE,
} batch2d_type_t;

typedef struct
{
  wt_vec2f_t rc_window_size;
  char pad[8];
  wt_mat4x4_t projection;
  wt_mat4x4_t view;
  wt_mat4x4_t model;
} ren_cbuffer_t;

typedef struct
{
  gpu_buffer_t buffer;
  usize capacity;
  gpu_buffer_desc_t desc;
} stretchy_buffer_t;

// format:
// 5 bits - x
// 5 bits - z
// 8 bits - y
// 8 bits - block
// 2 bits - texcoord index
// 4 bits - light level
typedef u32 chunk_vertex_t;

typedef struct
{
  wt_vec2f_t position;
  wt_vec2f_t rc_atlas_size;
} chunk_cbuffer_t;

typedef struct
{
  void *buffer;
  u64 num_bytes;
  bool is_dynamic_buffer;
} chunk_data_footer_t;

struct ren_chunk_t
{
  stretchy_buffer_t vertex_buffer, index_buffer;
  gpu_buffer_t const_buffer;
  usize num_vertices, num_indices;
  wt_vec2_t position;
};

typedef struct
{
  struct
  {
    solid2d_vertex_t vertices[MAX_SOLID2D_VERTICES];
    u16 indices[MAX_SOLID2D_INDICES];
    usize num_vertices, num_indices;
    gpu_buffer_t vertex_buffer, index_buffer;
    gpu_shader_t shader;
  } solid2d;

  struct
  {
    sprite2d_command_t commands[MAX_SPRITE_COMMANDS];
    usize num_commands;

    gpu_buffer_t cmd_buffer;
    gpu_buffer_t const_buffer;

    gpu_shader_t shader;
    ren_texture_t texture;
  } sprite2d;

  struct
  {
    wt_pool_t pool;

    // producer-consumer pattern - worker threads push chunk vertex/index data to this thread,
    // main thread updates the GPU buffers
    // producer pushes the data followed by a footer
    wt_arena_t data_arena;
    sys_mutex_t data_arena_mutex;

    gpu_shader_t shader;
    ren_texture_t atlas;
  } chunks;

  gpu_buffer_t const_buffer;
  wt_mat4x4_t view_matrix;

  batch2d_type_t last_batch2d;
} ren_state_t;

static ren_state_t *get_state(void)
{
  return game_get_state()->modules.ren;
}

void ren_init(void)
{
  game_state_t *gs = game_get_state();
  ren_state_t *s = gs->modules.ren = mem_hunk_push(sizeof(ren_state_t));

  s->const_buffer = gpu_buffer_new(&(gpu_buffer_desc_t){
      .type = GPU_BUFFER_CONSTANT,
      .size = sizeof(ren_cbuffer_t),
      .dynamic = true,
    });

  // === solid 2D geometry ===
  {
    s->solid2d.vertex_buffer = gpu_buffer_new(&(gpu_buffer_desc_t){
        .type = GPU_BUFFER_VERTEX,
        .size = sizeof(s->solid2d.vertices),
        .dynamic = true,
        .stride = sizeof(solid2d_vertex_t),
      });
    s->solid2d.index_buffer = gpu_buffer_new(&(gpu_buffer_desc_t){
        .type = GPU_BUFFER_INDEX,
        .size = sizeof(s->solid2d.indices),
        .dynamic = true,
        .index_type = GPU_INDEX_U16,
      });
    s->solid2d.shader = gpu_shader_new(&(gpu_shader_desc_t){
        .vs_path = "data/shaders/dx11/solid2d.vs.cso",
        .ps_path = "data/shaders/dx11/solid2d.ps.cso",
        .inputs[0] = { .type = GPU_DATA_FLOAT2     },
        .inputs[1] = { .type = GPU_DATA_BYTE4_NORM },
      });
  }

  // === sprites ===
  {
    s->sprite2d.shader = gpu_shader_new(&(gpu_shader_desc_t){
        .vs_path = "data/shaders/dx11/sprite2d.vs.cso",
        .ps_path = "data/shaders/dx11/sprite2d.ps.cso",
      });

    s->sprite2d.cmd_buffer = gpu_buffer_new(&(gpu_buffer_desc_t){
        .type = GPU_BUFFER_STRUCTS,
        .stride = sizeof(sprite2d_command_t),
        .dynamic = true,
        .size = sizeof(s->sprite2d.commands),
      });
    s->sprite2d.const_buffer = gpu_buffer_new(&(gpu_buffer_desc_t){
        .type = GPU_BUFFER_CONSTANT,
        .dynamic = true,
        .size = sizeof(sprite2d_cbuffer_t),
      });
  }

  // === chunks ===
  {
    s->chunks.shader = gpu_shader_new(&(gpu_shader_desc_t){
        .vs_path = "data/shaders/dx11/chunk.vs.cso",
        .ps_path = "data/shaders/dx11/chunk.ps.cso",
        .inputs[0] = { .type = GPU_DATA_UINT },
      });

    void *buffer = mem_hunk_push(CHUNK_MAX * wt_align16(sizeof(struct ren_chunk_t)));
    s->chunks.pool = wt_pool_new(buffer, CHUNK_MAX, sizeof(struct ren_chunk_t));

    s->chunks.data_arena = wt_arena_new(mem_hunk_push(CHUNK_DATA_ARENA_SIZE), CHUNK_DATA_ARENA_SIZE);
    s->chunks.data_arena_mutex = sys_mutex_new();

    s->chunks.atlas = ren_texture_load_from_file("data/tiles.png");
  }
}

void ren_frame_begin(wt_vec2_t window_size)
{
  wt_vec2_t ws = sys_window_get_size();

  ren_state_t *s = get_state();
  ren_cbuffer_t cbuffer = { 0 };
  cbuffer.rc_window_size = wt_vec2f_div(wt_vec2f(1, 1), wt_vec2i_to_vec2f(ws));
  cbuffer.projection = wt_mat4x4_perspective(0.2f, (f32)window_size.x / (f32)window_size.y, 0.001f, 10000.0f);
  cbuffer.view = s->view_matrix;
  gpu_buffer_update(s->const_buffer, &cbuffer, sizeof(cbuffer));

  gpu_frame_begin();
  gpu_clear(wt_color(138, 209, 235, 255));
  gpu_buffer_bind(s->const_buffer, 0);
}

ren_texture_t ren_texture_new(wt_color_t *pixels, int width, int height)
{
  ren_texture_t res = { 0 };
  res.width = width;
  res.height = height;
  res.texture = gpu_texture_new(&(gpu_texture_desc_t){
      .width = width,
      .height = height,
      .pixels = pixels,
    });
  return res;
}

ren_texture_t ren_texture_load_from_file(const char *filename)
{
  int width, height;
  wt_color_t *pixels = (wt_color_t*)stbi_load(filename, &width, &height, NULL, 4);
  if (pixels)
  {
    return ren_texture_new(pixels, width, height);
  }
  return (ren_texture_t){ 0 };
}

void ren_texture_free(ren_texture_t tx)
{
  gpu_texture_free(tx.texture);
}

static stretchy_buffer_t stretchy_buffer_new(gpu_buffer_desc_t *desc)
{
  stretchy_buffer_t res = { 0 };
  res.capacity = 1024;
  res.desc = *desc;
  return res;
}

static void stretchy_buffer_update(stretchy_buffer_t *sb, void *data, usize num_bytes)
{
  if (num_bytes == 0) { return; }

  if (sb->buffer == NULL || sb->capacity < num_bytes)
  {
    sb->capacity = (num_bytes + 0xfff) & ~0xfff;
    if (sb->buffer)
    {
      gpu_buffer_free(sb->buffer);
    }
    sb->buffer = gpu_buffer_new(&(gpu_buffer_desc_t){
        .size = sb->capacity,
        .type = sb->desc.type,
        .stride = sb->desc.stride,
        .index_type = sb->desc.index_type,
        .dynamic = true,
      });
  }
  gpu_buffer_update(sb->buffer, data, num_bytes);
}

ren_chunk_t ren_chunk_new(wt_vec2_t position)
{
  ren_state_t *s = get_state();
  ren_chunk_t res = wt_pool_alloc(&s->chunks.pool);
  res->position = position;

  res->vertex_buffer = stretchy_buffer_new(&(gpu_buffer_desc_t){
      .type = GPU_BUFFER_VERTEX,
      .stride = sizeof(chunk_vertex_t),
    });
  res->index_buffer = stretchy_buffer_new(&(gpu_buffer_desc_t){
      .type = GPU_BUFFER_INDEX,
      .index_type = GPU_INDEX_U32,
    });

  res->const_buffer = gpu_buffer_new(&(gpu_buffer_desc_t){
      .size = sizeof(chunk_cbuffer_t),
      .type = GPU_BUFFER_CONSTANT,
      .dynamic = true
    });

  return res;
}

block_id_t world_get_block(wt_vec3_t pos);
void ren_chunk_generate_mesh(ren_chunk_t c, block_id_t *blocks)
{
  ren_state_t *s = get_state();
  mem_scratch_begin();

  usize vertices_num_bytes = sizeof(chunk_vertex_t) * 4 * 6 * CHUNK_NUM_BLOCKS;
  usize indices_num_bytes = sizeof(u32) * 6 * 6 * CHUNK_NUM_BLOCKS;

  chunk_vertex_t *vertices = mem_scratch_push(vertices_num_bytes);
  u32 *indices = mem_scratch_push(indices_num_bytes);

  usize num_vertices = 0;
  usize num_indices = 0;

  bool shadows[CHUNK_SIZE_X * CHUNK_SIZE_Z] = { 0 };

//  for (usize i = 0; i < CHUNK_NUM_BLOCKS; ++i)
  for (isize i = CHUNK_NUM_BLOCKS - 1; i >= 0; --i)
  {
    if (blocks[i] == 0)
    {
      continue;
    }

    block_info_t *info = block_get_info(blocks[i]);
    WT_ASSERT(info);

    u32 top_idx = info->atlas_tiles[0].x + info->atlas_tiles[0].y * 16;
    u32 bot_idx = info->atlas_tiles[1].x + info->atlas_tiles[1].y * 16;
    u32 fnt_idx = info->atlas_tiles[2].x + info->atlas_tiles[2].y * 16;
    u32 bck_idx = info->atlas_tiles[3].x + info->atlas_tiles[3].y * 16;
    u32 lft_idx = info->atlas_tiles[4].x + info->atlas_tiles[4].y * 16;
    u32 rgt_idx = info->atlas_tiles[5].x + info->atlas_tiles[5].y * 16;

    typedef struct { wt_vec3_t pos; u32 block; u32 texcoord; u32 light; } unpacked_vertex_t;

    u8 bri = 15;
    u8 mid = 7;
    u8 mid2 = 9;
    u8 dim = 4;

    unpacked_vertex_t block_vertices[] = {
      // top face
      { { 0, 1, 0 }, top_idx, 0, bri },
      { { 1, 1, 0 }, top_idx, 1, bri },
      { { 0, 1, 1 }, top_idx, 2, bri },
      { { 1, 1, 1 }, top_idx, 3, bri },

      // bottom face
      { { 0, 0, 0 }, bot_idx, 0, dim },
      { { 1, 0, 0 }, bot_idx, 1, dim },
      { { 0, 0, 1 }, bot_idx, 2, dim },
      { { 1, 0, 1 }, bot_idx, 3, dim },

      // front face
      { { 0, 0, 0 }, fnt_idx, 2, mid },
      { { 1, 0, 0 }, fnt_idx, 3, mid },
      { { 0, 1, 0 }, fnt_idx, 0, mid },
      { { 1, 1, 0 }, fnt_idx, 1, mid },

      // back face
      { { 0, 0, 1 }, bck_idx, 2, mid },
      { { 1, 0, 1 }, bck_idx, 3, mid },
      { { 0, 1, 1 }, bck_idx, 0, mid },
      { { 1, 1, 1 }, bck_idx, 1, mid },

      // left face
      { { 0, 0, 0 }, lft_idx, 3, mid2 },
      { { 0, 1, 0 }, lft_idx, 1, mid2 },
      { { 0, 0, 1 }, lft_idx, 2, mid2 },
      { { 0, 1, 1 }, lft_idx, 0, mid2 },

      // right face
      { { 1, 0, 0 }, rgt_idx, 2, mid2 },
      { { 1, 1, 0 }, rgt_idx, 0, mid2 },
      { { 1, 0, 1 }, rgt_idx, 3, mid2 },
      { { 1, 1, 1 }, rgt_idx, 1, mid2 },
    };
    u32 block_indices[] = {
      0, 1, 3,
      3, 2, 0,

      3, 1, 0,
      0, 2, 3,

      0, 1, 3,
      3, 2, 0,

      3, 1, 0,
      0, 2, 3,

      0, 1, 3,
      3, 2, 0,

      3, 1, 0,
      0, 2, 3,
    };

    f32 fi = i;
    wt_vec3f_t block_coords = wt_vec3f(
      fmodf(fi, CHUNK_SIZE_X),
      fi / (CHUNK_SIZE_X * CHUNK_SIZE_Z),
      fmodf((fi / CHUNK_SIZE_X), CHUNK_SIZE_Z));

    wt_vec3_t neighbors[] = {
      {  0,  1,  0 },
      {  0, -1,  0 },
      {  0,  0, -1 },
      {  0,  0,  1 },
      { -1,  0,  0 },
      {  1,  0,  0 },
    };

    bool *shadow = &shadows[(int)block_coords.x + (int)block_coords.z * CHUNK_SIZE_X];
    if (*shadow)
    {
      block_vertices[0].light = WT_MAX(0, (int)block_vertices[0].light - 5);
      block_vertices[1].light = WT_MAX(0, (int)block_vertices[1].light - 5);
      block_vertices[2].light = WT_MAX(0, (int)block_vertices[2].light - 5);
      block_vertices[3].light = WT_MAX(0, (int)block_vertices[3].light - 5);
    }
    *shadow = true;

    for (usize j = 0; j < 6; ++j)
    {
      wt_vec3_t neighbor_offset = neighbors[j];
      wt_vec3_t neighbor = wt_vec3i_add(wt_vec3f_to_vec3i(block_coords), neighbor_offset);

      // if the block has a solid neighbor, we can skip drawing this face
      bool out_of_bounds =
        (neighbor.x < 0 || neighbor.x >= CHUNK_SIZE_X) ||
        (neighbor.y < 0 || neighbor.y >= CHUNK_SIZE_Y) ||
        (neighbor.z < 0 || neighbor.z >= CHUNK_SIZE_Z);

      block_id_t neighbor_id = 0;
      if (neighbor.y >= 0)
      {
        if (out_of_bounds)
        {
          wt_vec2_t chunk_pos = wt_vec2i_mul_i32(c->position, 16);
          neighbor.x += chunk_pos.x;
          neighbor.z += chunk_pos.y;
          if (neighbor.y >= 0 && neighbor.y < CHUNK_SIZE_Y)
          {
            neighbor_id = world_get_block(neighbor);
          }
        }
        else
        {
          neighbor_id = blocks[neighbor.x + (neighbor.z * CHUNK_SIZE_Z) +
            (neighbor.y * CHUNK_SIZE_X * CHUNK_SIZE_Z)];
        }
      }

      if (neighbor_id != 0)
      {
        block_info_t *neighbor_info = block_get_info(neighbor_id);
        if (!neighbor_info || neighbor_info->solid)
        {
          continue;
        }
      }

      for (usize k = 0; k < 6; ++k)
      {
        indices[num_indices++] = block_indices[j * 6 + k] + num_vertices;
      }

      for (usize k = 0; k < 4; ++k)
      {
        unpacked_vertex_t u = block_vertices[j * 4 + k];
        chunk_vertex_t p = 0;
        u.pos = wt_vec3i_add(u.pos, wt_vec3f_to_vec3i(block_coords));
        p |= u.pos.x    << (32 - 5);
        p |= u.pos.z    << (32 - 10);
        p |= u.pos.y    << (32 - 18);
        p |= u.block    << (32 - 26);
        p |= u.texcoord << (32 - 28);
        p |= u.light    << (32 - 32);
        vertices[num_vertices++] = p;
      }
    }
  }

  chunk_cbuffer_t cbuffer_data = { 0 };
  cbuffer_data.position = wt_vec2f(c->position.x * CHUNK_SIZE_X, c->position.y * CHUNK_SIZE_Z);
  cbuffer_data.rc_atlas_size = wt_vec2f_div(wt_vec2f(1.0f, 1.0f), wt_vec2f(256.0f, 256.0f));

  // send buffer data to main thread via arena
  sys_mutex_lock(s->chunks.data_arena_mutex);
  {
    chunk_data_footer_t footer = { 0 };

    footer.buffer = c->const_buffer;
    footer.num_bytes = sizeof(cbuffer_data);
    wt_arena_push_from(&s->chunks.data_arena, &cbuffer_data, sizeof(cbuffer_data));
    wt_arena_push_from(&s->chunks.data_arena, &footer, sizeof(footer));

    footer.buffer = &c->vertex_buffer;
    footer.num_bytes = num_vertices * sizeof(chunk_vertex_t);
    footer.is_dynamic_buffer = true;
    wt_arena_push_from(&s->chunks.data_arena, vertices, footer.num_bytes);
    wt_arena_push_from(&s->chunks.data_arena, &footer, sizeof(footer));

    footer.buffer = &c->index_buffer;
    footer.num_bytes = num_indices * sizeof(u32);
    footer.is_dynamic_buffer = true;
    wt_arena_push_from(&s->chunks.data_arena, indices, footer.num_bytes);
    wt_arena_push_from(&s->chunks.data_arena, &footer, sizeof(footer));

    sys_mutex_unlock(s->chunks.data_arena_mutex);
  }

  c->num_vertices = num_vertices;
  c->num_indices = num_indices;

  mem_scratch_end();
}

void ren_chunk_free(ren_chunk_t c)
{
  ren_state_t *s = get_state();
  gpu_buffer_free(c->vertex_buffer.buffer);
  gpu_buffer_free(c->index_buffer.buffer);
  gpu_buffer_free(c->const_buffer);
  wt_pool_free(&s->chunks.pool, c);
}

void ren_camera_set(wt_mat4x4_t mtx)
{
  ren_state_t *s = get_state();
  s->view_matrix = mtx;
}

static void solid2d_flush(void)
{
  ren_state_t *s = get_state();
  if (s->solid2d.num_vertices > 0 && s->solid2d.num_indices > 0)
  {
    gpu_depth_disable();

    gpu_buffer_update(s->solid2d.vertex_buffer, s->solid2d.vertices,
      s->solid2d.num_vertices * sizeof(solid2d_vertex_t));
    gpu_buffer_update(s->solid2d.index_buffer, s->solid2d.indices, s->solid2d.num_indices * sizeof(u16));

    gpu_buffer_bind(s->solid2d.vertex_buffer, 0);
    gpu_buffer_bind(s->solid2d.index_buffer, 0);
    gpu_shader_bind(s->solid2d.shader);
    gpu_draw_indexed(GPU_PRIMITIVE_TRIANGLES, 0, s->solid2d.num_indices);

    s->solid2d.num_vertices = 0;
    s->solid2d.num_indices = 0;

    gpu_depth_enable();
  }
}

static void sprite2d_flush(void)
{
  ren_state_t *s = get_state();
  if (s->sprite2d.num_commands > 0)
  {
    gpu_buffer_update(s->sprite2d.cmd_buffer, s->sprite2d.commands,
      s->sprite2d.num_commands * sizeof(sprite2d_command_t));
    gpu_buffer_bind(s->sprite2d.cmd_buffer, 0);
    gpu_buffer_bind(s->sprite2d.const_buffer, 1);
    gpu_texture_bind(s->sprite2d.texture.texture, 1);
    gpu_shader_bind(s->sprite2d.shader);
    gpu_draw_instanced(GPU_PRIMITIVE_TRIANGLE_STRIP, 0, 4, s->sprite2d.num_commands);

    s->sprite2d.num_commands = 0;
  }
}

static void flush2d(void)
{
  ren_state_t *s = get_state();
  switch (s->last_batch2d)
  {
  case BATCH2D_SOLID:
    solid2d_flush();
    break;
  case BATCH2D_SPRITE:
    sprite2d_flush();
    break;
  default: break;
  }
}

static void begin_2d_primitive(batch2d_type_t type)
{
  ren_state_t *s = get_state();
  if (type != s->last_batch2d)
  {
    flush2d();
    s->last_batch2d = type;
  }
}

static void solid2d_add_primitive(solid2d_vertex_t *vertices, u16 *indices,
  usize num_vertices, usize num_indices)
{
  ren_state_t *s = get_state();

  begin_2d_primitive(BATCH2D_SOLID);

  usize actual_num_indices = (num_indices == 0) ? num_vertices : num_indices;
  if (s->solid2d.num_vertices + num_vertices >= MAX_SOLID2D_VERTICES ||
    s->solid2d.num_indices + actual_num_indices >= MAX_SOLID2D_INDICES)
  {
    flush2d();
  }

  for (usize i = 0; i < actual_num_indices; ++i)
  {
    if (indices == NULL)
    {
      s->solid2d.indices[s->solid2d.num_indices + i] = s->solid2d.num_vertices + i;
    }
    else
    {
      s->solid2d.indices[s->solid2d.num_indices + i] = s->solid2d.num_vertices + indices[i];
    }
  }

  for (usize i = 0; i < num_vertices; ++i)
  {
    s->solid2d.vertices[s->solid2d.num_vertices + i] = vertices[i];
  }

  s->solid2d.num_vertices += num_vertices;
  s->solid2d.num_indices += actual_num_indices;
}

static void sprite2d_add_sprite(ren_texture_t texture, sprite2d_command_t cmd)
{
  ren_state_t *s = get_state();
  begin_2d_primitive(BATCH2D_SPRITE);

  if (s->sprite2d.num_commands + 1 > MAX_SPRITE_COMMANDS)
  {
    flush2d();
  }

  if (s->sprite2d.texture.texture != texture.texture)
  {
    flush2d();

    s->sprite2d.texture = texture;

    sprite2d_cbuffer_t cb = { 0 };
    cb.rc_atlas_size = wt_vec2f_div(wt_vec2f(1, 1), wt_vec2f(texture.width, texture.height));
    gpu_buffer_update(s->sprite2d.const_buffer, &cb, sizeof(cb));
  }

  s->sprite2d.commands[s->sprite2d.num_commands++] = cmd;
}

void ren_draw_box(wt_rect_t rect, wt_color_t color)
{
  ren_draw_rect(wt_rect(rect.x,              rect.y,              rect.w, 1     ), color);
  ren_draw_rect(wt_rect(rect.x,              rect.y,              1,      rect.h), color);
  ren_draw_rect(wt_rect(rect.x,              rect.y + rect.h - 1, rect.w, 1     ), color);
  ren_draw_rect(wt_rect(rect.x + rect.w - 1, rect.y,              1,      rect.h), color);
}

void ren_draw_rect(wt_rect_t rect, wt_color_t color)
{
  solid2d_vertex_t vertices[] = {
    { { rect.x,          rect.y          }, color },
    { { rect.x + rect.w, rect.y          }, color },
    { { rect.x,          rect.y + rect.h }, color },
    { { rect.x + rect.w, rect.y + rect.h }, color },
  };
  u16 indices[] = { 0, 1, 3, 3, 2, 0 };
  solid2d_add_primitive(vertices, indices, WT_ARRAY_COUNT(vertices), WT_ARRAY_COUNT(indices));
}

void ren_frame_end(void)
{
  ren_state_t *s = get_state();

  // update any chunk meshes received from the worker threads
  sys_mutex_lock(s->chunks.data_arena_mutex);
  while (s->chunks.data_arena.pos > 0)
  {
    chunk_data_footer_t footer = { 0 };
    memcpy(&footer, wt_arena_get_last(&s->chunks.data_arena, sizeof(footer)), sizeof(footer));
    wt_arena_pop(&s->chunks.data_arena, sizeof(footer));

    void *data = wt_arena_get_last(&s->chunks.data_arena, footer.num_bytes);
    if (footer.is_dynamic_buffer)
    {
      stretchy_buffer_update(footer.buffer, data, footer.num_bytes);
    }
    else
    {
      gpu_buffer_update(footer.buffer, data, footer.num_bytes);
    }

    wt_arena_pop(&s->chunks.data_arena, footer.num_bytes);
  }

  sys_mutex_unlock(s->chunks.data_arena_mutex);

  flush2d();
  gpu_frame_end();
}

void ren_draw_sprite(ren_texture_t tx, wt_vec2_t pos)
{
  ren_draw_sprite_scaled(tx, pos, 1);
}

void ren_draw_sprite_scaled(ren_texture_t tx, wt_vec2_t pos, i32 scale)
{
  ren_draw_sprite_stretch(tx, wt_rect(0, 0, tx.width, tx.height),
    wt_rect(pos.x, pos.y, tx.width * scale, tx.height * scale));
}

void ren_draw_sprite_stretch(ren_texture_t tx, wt_rect_t src, wt_rect_t dst)
{
  ren_draw_sprite_ex(tx, src, dst, wt_vec2(0, 0), 0.0f, wt_color(255, 255, 255, 255));
}

void ren_draw_sprite_ex(ren_texture_t tx, wt_rect_t src, wt_rect_t dst,
  wt_vec2_t origin, float rotation, wt_color_t tint)
{
  WT_UNUSED(origin);
  WT_UNUSED(tint);

  sprite2d_add_sprite(tx, (sprite2d_command_t){
      .dst = dst,
      .src = src,
      .rotation = rotation,
    });
}

void ren_draw_chunk(ren_chunk_t c)
{
  ren_state_t *s = get_state();
  if (c->vertex_buffer.buffer && c->index_buffer.buffer)
  {
    flush2d();

    gpu_buffer_bind(c->vertex_buffer.buffer, 0);
    gpu_buffer_bind(c->index_buffer.buffer, 0);
    gpu_buffer_bind(c->const_buffer, 1);
    gpu_shader_bind(s->chunks.shader);
    gpu_texture_bind(s->chunks.atlas.texture, 0);
    gpu_draw_indexed(GPU_PRIMITIVE_TRIANGLES, 0, c->num_indices);
  }
}
