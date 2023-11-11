#include "game.h"
#include "memory.h"
#include "system.h"
#include "renderer.h"
#include "constants.h"
#include "rng.h"
#include "job.h"
#include "chunk.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

game_state_t *s_state;

static void chunk_gen_job(void *param)
{
  chunk_t *chunk = (chunk_t*)param;
  chunk_gen(chunk);
}

static void generate_world(void)
{
  game_state_t *s = s_state;
  for (u32 i = 0; i < WT_ARRAY_COUNT(s->chunks); ++i)
  {
    job_queue(chunk_gen_job, s->chunks[i]);
  }
}

void game_init(game_state_t *s)
{
  s_state = s;

  mem_init();
  sys_init();
  job_init();
  mem_post_init();
  ren_init();

  rng_init();

  sys_mouse_capture();

  block_mgr_init();
  chunk_init();

  block_info_t blocks_info[] = {
    { "Air",        
      { wt_vec2(0, 0), wt_vec2(0, 0), wt_vec2(0, 0), wt_vec2(0, 0), wt_vec2(0, 0), wt_vec2(0, 0) },
      false },
    { "Dirt",        
      { wt_vec2(1, 0), wt_vec2(1, 0), wt_vec2(1, 0), wt_vec2(1, 0), wt_vec2(1, 0), wt_vec2(1, 0) },
      true  },
    { "Grass Block", 
      { wt_vec2(3, 0), wt_vec2(1, 0), wt_vec2(2, 0), wt_vec2(2, 0), wt_vec2(2, 0), wt_vec2(2, 0) },
      true  },
    { "Planks",      
      { wt_vec2(1, 1), wt_vec2(1, 1), wt_vec2(1, 1), wt_vec2(1, 1), wt_vec2(1, 1), wt_vec2(1, 1) },
      true  },
    { "Cobblestone", 
      { wt_vec2(2, 1), wt_vec2(2, 1), wt_vec2(2, 1), wt_vec2(2, 1), wt_vec2(2, 1), wt_vec2(2, 1) },
      true  },
    { "Stone Tile",  
      { wt_vec2(3, 1), wt_vec2(3, 1), wt_vec2(3, 1), wt_vec2(3, 1), wt_vec2(3, 1), wt_vec2(3, 1) },
      true  },
    { "Log",  
      { wt_vec2(4, 0), wt_vec2(4, 0), wt_vec2(4, 1), wt_vec2(4, 1), wt_vec2(4, 1), wt_vec2(4, 1) },
      true  },
    { "Leaves",  
      { wt_vec2(5, 0), wt_vec2(5, 0), wt_vec2(5, 0), wt_vec2(5, 0), wt_vec2(5, 0), wt_vec2(5, 0) },
      false },
  };

  for (usize i = 0; i < WT_ARRAY_COUNT(blocks_info); ++i)
  {
    s->blocks[i] = block_new(&blocks_info[i]);
  }

  s->debug_font = ren_texture_load_from_file("data/sprites/debug-font.png");

  for (usize i = 0; i < WT_ARRAY_COUNT(s->chunks); ++i)
  {
    s->chunks[i] = chunk_new(wt_vec2(i % 64, i / 64));
  }

  s->camera_pos.x = 256;
  s->camera_pos.y = 15;
  s->camera_pos.z = 256;

  s->hotbar[0] = s->blocks[BLOCK_DIRT];
  s->hotbar[1] = s->blocks[BLOCK_GRASS_BLOCK];
  s->hotbar[2] = s->blocks[BLOCK_PLANKS];
  s->hotbar[3] = s->blocks[BLOCK_COBBLESTONE];
  s->hotbar[4] = s->blocks[BLOCK_TILE];
  s->hotbar[5] = s->blocks[BLOCK_LOG];
  s->hotbar[6] = s->blocks[BLOCK_LEAVES];

  generate_world();
}

static void world_set_block(wt_vec3_t pos, block_id_t block)
{
  game_state_t *s = s_state;

  wt_vec2_t chunk_pos = { 0 };
  chunk_pos.x = floorf(pos.x / CHUNK_SIZE_X);
  chunk_pos.y = floorf(pos.z / CHUNK_SIZE_Z);
  if (chunk_pos.x >= 0 && chunk_pos.y >= 0 &&
    chunk_pos.x < 64 && chunk_pos.y < 64)
  {
    chunk_t *c = s->chunks[chunk_pos.x + chunk_pos.y * 64];

    wt_vec3_t block_pos = { 0 };
    block_pos.x = pos.x % CHUNK_SIZE_X;
    block_pos.y = pos.y;
    block_pos.z = pos.z % CHUNK_SIZE_Z;
    
    chunk_set_block(c, block_pos, block);

    // if we're breaking a block at the edge of a chunk, we need to update the neighboring chunk
    if (block == 0)
    {
      if (block_pos.x == 0 && chunk_pos.x > 0)
      {
        s->chunks[(chunk_pos.x - 1) + chunk_pos.y * 64]->dirty = true;
      }
      if (block_pos.x == CHUNK_SIZE_X - 1 && chunk_pos.x < 64 - 1)
      {
        s->chunks[(chunk_pos.x + 1) + chunk_pos.y * 64]->dirty = true;
      }
      if (block_pos.z == 0 && chunk_pos.y > 0)
      {
        s->chunks[chunk_pos.x + (chunk_pos.y - 1) * 64]->dirty = true;
      }
      if (block_pos.z == CHUNK_SIZE_X - 1 && chunk_pos.y < 64 - 1)
      {
        s->chunks[chunk_pos.x + (chunk_pos.y + 1) * 64]->dirty = true;
      }
    }
  }
}

block_id_t world_get_block(wt_vec3_t pos)
{
  game_state_t *s = s_state;

  if (pos.x < 0 || pos.y < 0 || pos.z < 0)
  {
    return 0;
  }
  
  wt_vec2_t chunk_pos = { 0 };
  chunk_pos.x = floorf(pos.x / CHUNK_SIZE_X);
  chunk_pos.y = floorf(pos.z / CHUNK_SIZE_Z);
  wt_vec3_t block_pos = { 0 };
  block_pos.x = pos.x % CHUNK_SIZE_X;
  block_pos.y = pos.y;
  block_pos.z = pos.z % CHUNK_SIZE_Z;

  if (chunk_pos.x >= 0 && chunk_pos.y >= 0 && chunk_pos.x < 64 && chunk_pos.y < 64)
  {  
    chunk_t *c = s->chunks[chunk_pos.x + chunk_pos.y * 64];
    return chunk_get_block(c, block_pos);
  }
  return 0;
}

static wt_vec3f_t world_get_player_pos(void)
{
  // i do not know why it's multiplied by two...
  game_state_t *s = s_state;
  wt_vec3f_t res = wt_vec3f_mul_f32(s->camera_pos, 2.0f);
  res.y -= 1.0f; // body instead of head
  return res;
}

typedef struct
{
  wt_vec3_t pos;
  wt_vec3_t neighbor;

  block_face_t face;
  
  bool hit;
} world_raycast_t;

static world_raycast_t world_raycast(int max_num_blocks)
{
  game_state_t *s = s_state;

  wt_vec3f_t player_pos = world_get_player_pos();
  player_pos.y += 1; // cast from the player's head  
  wt_vec3i_t map_pos = wt_vec3f_to_vec3i(player_pos);

  f32 yc = cosf(s->camera_rotation.y * WT_TAU_F32);
  f32 ys = sinf(s->camera_rotation.y * WT_TAU_F32);
  f32 ac = cosf(s->camera_rotation.x * WT_TAU_F32) * yc;
  f32 as = sinf(s->camera_rotation.x * WT_TAU_F32) * yc;

  // length of ray from one side to the next side
  f32 delta_dist_x = (ac == 0.0f) ? 1e30 : fabsf(1.0f / ac);
  f32 delta_dist_y = (ys == 0.0f) ? 1e30 : fabsf(1.0f / ys);
  f32 delta_dist_z = (as == 0.0f) ? 1e30 : fabsf(1.0f / as);

  f32 side_dist_x, side_dist_y, side_dist_z; // length of ray from current position to next xyz side
  i32 step_x, step_y, step_z; // -1 or +1 - which direction to advance the ray

  if (ac > 0) { step_x = +1; side_dist_x = (map_pos.x + 1.0f - player_pos.x) * delta_dist_x; }
  else        { step_x = -1; side_dist_x = (player_pos.x - map_pos.x)        * delta_dist_x; }
  if (ys > 0) { step_y = +1; side_dist_y = (map_pos.y + 1.0f - player_pos.y) * delta_dist_y; }
  else        { step_y = -1; side_dist_y = (player_pos.y - map_pos.y)        * delta_dist_y; }
  if (as > 0) { step_z = +1; side_dist_z = (map_pos.z + 1.0f - player_pos.z) * delta_dist_z; }
  else        { step_z = -1; side_dist_z = (player_pos.z - map_pos.z)        * delta_dist_z; }

  bool hit = false;

  int max_dist_sq = max_num_blocks * max_num_blocks * 3;
  
  wt_vec3i_t prev_map_pos = { 0 };
  for (int limit = 0; limit < 100000; ++limit)
  {
    // advance the ray
    prev_map_pos = map_pos;
    if (side_dist_x < side_dist_z && side_dist_x < side_dist_y)
    {
      side_dist_x += delta_dist_x;
      map_pos.x += step_x;
    }
    else if (side_dist_y < side_dist_x && side_dist_y < side_dist_z)
    {
      side_dist_y += delta_dist_y;
      map_pos.y += step_y;
    }
    else
    {
      side_dist_z += delta_dist_z;
      map_pos.z += step_z;
    }

    // make sure we haven't hit the distance cap
    wt_vec3i_t diff = wt_vec3i_sub(map_pos, wt_vec3f_to_vec3i(player_pos));
    diff.x = abs(diff.x);
    diff.y = abs(diff.y);
    diff.z = abs(diff.z);
    if (diff.x*diff.x + diff.y*diff.y + diff.z*diff.z > max_dist_sq)
    {
      break;
    }

    // check for hits
    block_id_t block = world_get_block(map_pos);
    if (block != 0)
    {
      hit = true;
      break;
    }
  }

  world_raycast_t raycast = { 0 };
  raycast.pos = map_pos;
  raycast.neighbor = prev_map_pos;

  wt_vec3_t face_offset = wt_vec3i_sub(raycast.neighbor, raycast.pos);
  if (face_offset.x == +1) { raycast.face = BLOCK_FACE_RIGHT;  }
  if (face_offset.x == -1) { raycast.face = BLOCK_FACE_LEFT;   }
  if (face_offset.y == +1) { raycast.face = BLOCK_FACE_TOP;    }
  if (face_offset.y == -1) { raycast.face = BLOCK_FACE_BOTTOM; }
  if (face_offset.z == +1) { raycast.face = BLOCK_FACE_BACK;   }
  if (face_offset.z == -1) { raycast.face = BLOCK_FACE_FRONT;  }

  raycast.hit = hit;

  return raycast;
}

static void game_render(void);
bool game_tick(game_state_t *s)
{
  s_state = s;
  bool should_close = sys_should_close();
  if (should_close)
  {
    return false;
  }

  sys_poll_events();

  if (sys_key_pressed(SYS_KEYCODE_G))
  {
    generate_world();
  }
  
  job_tick();
  
  // === camera movement ===
  {
    static const f32 k_camera_speed = 0.1f;
    static const f32 k_camera_run_speed = 0.2f;
    static const wt_vec3f_t k_camera_up = { 0, 1, 0 };

    wt_vec3f_t camera_front = { 0, 0, 1 };

    f32 speed = (sys_key_down(SYS_KEYCODE_LSHIFT)) ? k_camera_run_speed : k_camera_speed;

    if (sys_key_down(SYS_KEYCODE_SPACE))  { s->camera_pos.y += speed; }
    if (sys_key_down(SYS_KEYCODE_LCTRL)) { s->camera_pos.y -= speed; }

    wt_vec2_t mouse_delta = sys_mouse_get_delta();
    s->camera_rotation.x += (f32)mouse_delta.x / 1000.0f;
    s->camera_rotation.y -= (f32)mouse_delta.y / 1000.0f;

    s->camera_rotation.y = WT_CLAMP(s->camera_rotation.y, -0.24, +0.24);

    wt_vec3f_t front = { 0 };
    front.x = cosf(s->camera_rotation.x * WT_TAU_F32) * cosf(s->camera_rotation.y * WT_TAU_F32);
    front.y = sinf(s->camera_rotation.y * WT_TAU_F32);
    front.z = sinf(s->camera_rotation.x * WT_TAU_F32) * cosf(s->camera_rotation.y * WT_TAU_F32);
    camera_front = wt_vec3f_norm(front);

    if (sys_key_down(SYS_KEYCODE_W))
    {
      s->camera_pos = wt_vec3f_add(s->camera_pos, wt_vec3f_mul_f32(camera_front, speed));
    }
    if (sys_key_down(SYS_KEYCODE_S))
    {
      s->camera_pos = wt_vec3f_sub(s->camera_pos, wt_vec3f_mul_f32(camera_front, speed));
    }
  
    if (sys_key_down(SYS_KEYCODE_D))
    {
      s->camera_pos = wt_vec3f_add(s->camera_pos,
        wt_vec3f_mul_f32(wt_vec3f_norm(wt_vec3f_cross(camera_front, k_camera_up)), speed));
    }
    if (sys_key_down(SYS_KEYCODE_A))
    {
      s->camera_pos = wt_vec3f_sub(s->camera_pos,
        wt_vec3f_mul_f32(wt_vec3f_norm(wt_vec3f_cross(camera_front, k_camera_up)), speed));
    }

    s->camera_front = camera_front;

    wt_mat4x4_t direction = wt_mat4x4_look_at(s->camera_pos, wt_vec3f_add(s->camera_pos, camera_front),
      k_camera_up);

    ren_camera_set(direction);
  }

  i32 mouse_wheel = sys_mouse_get_wheel();
  if (mouse_wheel != 0)
  {
    s->hotbar_pos = (s->hotbar_pos - mouse_wheel) % INVENTORY_SIZE;
  }

  if (sys_mouse_pressed(SYS_MOUSE_RIGHT))
  {
    world_raycast_t rc = world_raycast(4);
    if (rc.hit)
    {
      world_set_block(rc.neighbor, s->hotbar[s->hotbar_pos]);
    }
  }

  if (sys_mouse_pressed(SYS_MOUSE_LEFT))
  {
    world_raycast_t rc = world_raycast(4);
    if (rc.hit)
    {
      world_set_block(rc.pos, 0);
    }
  }

  if (sys_mouse_pressed(SYS_MOUSE_MIDDLE))
  {
#if 0
    world_raycast_t rc = world_raycast(10);
    if (rc.hit)
    {
      block_id_t id = world_get_block(rc.pos);
      if (id)
      {
        s->player_holding = id;
      }
    }
#endif
  }

  if (sys_key_pressed(SYS_KEYCODE_R))
  {
    for (u32 i = 0; i < WT_ARRAY_COUNT(s->chunks); ++i)
    {
      chunk_rebuild_mesh(s->chunks[i]);
    }
  }

  if (sys_key_down(SYS_KEYCODE_ESCAPE))
  {
    return false;
  }

  game_render();
  return !should_close;
}

static void draw_debug_text(wt_vec2_t pos_cells, const char *fmt, ...)
{
  game_state_t *s = s_state;
  char buf[1024];
  va_list va;
  va_start(va, fmt);
  vsnprintf(buf, sizeof(buf), fmt, va);
  va_end(va);

  for (u32 i = 0; buf[i]; ++i)
  {
    wt_vec2_t pos = wt_vec2i_mul(pos_cells, wt_vec2(8, 16));
    wt_rect_t dst = wt_rect(pos.x, pos.y, 8, 16);
    wt_rect_t src = wt_rect((buf[i] % 16) * 8, (buf[i] / 16) * 16, 8, 16);

    ren_draw_sprite_stretch(s->debug_font, src, dst);
    pos_cells.x += 1;
  }
}

static void game_render(void)
{
  game_state_t *s = s_state;
  WT_UNUSED(s);

  wt_vec2_t window_size = sys_window_get_size();
  ren_frame_begin(window_size);

  static u64 timer;
  timer += 1;

  float f[6];
  for (int i = 0; i < 6; ++i)
  {
    f[i] = sin((f64)(timer + i * 10) / 20.0) * 10.0;
  }
  //float f = sin((f64)timer / 20.0) * 10.0;

  //ren_chunk_generate_mesh(s->test_chunk);
  for (usize i = 0; i < WT_ARRAY_COUNT(s->chunks); ++i)
  {
    chunk_render(s->chunks[i]);
  }

  wt_rect_t reticle[2] = {
    wt_rect(window_size.x / 2 - 1, window_size.y / 2 - 8, 2, 16),
    wt_rect(window_size.x / 2 - 8, window_size.y / 2 - 1, 16, 2),
  };
  ren_draw_rect(reticle[0], WT_COLOR_WHITE);
  ren_draw_rect(reticle[1], WT_COLOR_WHITE);
  
  draw_debug_text(wt_vec2(1, 1), "camera pos:   (%f %f %f)",
    s->camera_pos.x, s->camera_pos.y, s->camera_pos.z);
  draw_debug_text(wt_vec2(1, 2), "camera front: (%f %f %f)",
    s->camera_front.x, s->camera_front.y, s->camera_front.z);
  draw_debug_text(wt_vec2(1, 3), "camera rotation: (%.2f %.2f)",
    s->camera_rotation.x * 360.0f, s->camera_rotation.y * 360.0f);

  block_info_t *holding_info = block_get_info(s->hotbar[s->hotbar_pos]);
  if (holding_info)
  {
    draw_debug_text(wt_vec2(1, 4), "holding: %s", holding_info->name);
  }
  
  ren_frame_end();
}

void game_hot_unload(game_state_t *s)
{
  s_state = s;

  job_pause_all();
}

void game_hot_reload(game_state_t *s)
{
  s_state = s;

  job_resume_all();
}

game_state_t *game_get_state(void)
{
  return s_state;
}
