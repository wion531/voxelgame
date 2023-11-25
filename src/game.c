#include "game.h"
#include "memory.h"
#include "system.h"
#include "gpu.h"
#include "renderer.h"
#include "constants.h"
#include "rng.h"
#include "job.h"
#include "chunk.h"
#include "world.h"
#include "player.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRIPPED_DOWN 1

game_state_t *s_state;

typedef struct
{
  wt_vec2f_t rc_window_size;
  char pad[8];
  wt_mat4x4_t projection;
  wt_mat4x4_t view;
  wt_mat4x4_t model;
} const_buffer_data_t;

typedef struct
{
  wt_vec2f_t pos;
  wt_color_t color;
  wt_vec2f_t uv;
} vertex_t;

void game_init(game_state_t *s)
{
  s_state = s;

  mem_init();
  sys_init();
  job_init();
  mem_post_init();

#if STRIPPED_DOWN
  gpu_init();
  ren_init();
#else
  ren_init();

  rng_init();

  sys_mouse_capture();

  block_mgr_init();
  chunk_init();
  world_init();

  player_init();

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

    { "White Cloth",
      { wt_vec2(1, 3), wt_vec2(1, 3), wt_vec2(1, 3), wt_vec2(1, 3), wt_vec2(1, 3), wt_vec2(1, 3) },
      true },
    { "Red Cloth",
      { wt_vec2(2, 3), wt_vec2(2, 3), wt_vec2(2, 3), wt_vec2(2, 3), wt_vec2(2, 3), wt_vec2(2, 3) },
      true },
    { "Orange Cloth",
      { wt_vec2(3, 3), wt_vec2(3, 3), wt_vec2(3, 3), wt_vec2(3, 3), wt_vec2(3, 3), wt_vec2(3, 3) },
      true },
    { "Yellow Cloth",
      { wt_vec2(4, 3), wt_vec2(4, 3), wt_vec2(4, 3), wt_vec2(4, 3), wt_vec2(4, 3), wt_vec2(4, 3) },
      true },
    { "Green Cloth",
      { wt_vec2(5, 3), wt_vec2(5, 3), wt_vec2(5, 3), wt_vec2(5, 3), wt_vec2(5, 3), wt_vec2(5, 3) },
      true },
    { "Blue Cloth",
      { wt_vec2(6, 3), wt_vec2(6, 3), wt_vec2(6, 3), wt_vec2(6, 3), wt_vec2(6, 3), wt_vec2(6, 3) },
      true },
    { "Purple Cloth",
      { wt_vec2(7, 3), wt_vec2(7, 3), wt_vec2(7, 3), wt_vec2(7, 3), wt_vec2(7, 3), wt_vec2(7, 3) },
      true },
    { "Black Cloth",
      { wt_vec2(8, 3), wt_vec2(8, 3), wt_vec2(8, 3), wt_vec2(8, 3), wt_vec2(8, 3), wt_vec2(8, 3) },
      true },
  };

  for (usize i = 0; i < WT_ARRAY_COUNT(blocks_info); ++i)
  {
    s->blocks[i] = block_new(&blocks_info[i]);
  }

  s->debug_font = ren_texture_load_from_file("data/sprites/debug-font.png");

#if 0
  s->camera_pos.x = 256;
  s->camera_pos.y = 15;
  s->camera_pos.z = 256;
#endif

  s->hotbar[0] = s->blocks[BLOCK_DIRT];
  s->hotbar[1] = s->blocks[BLOCK_GRASS_BLOCK];
  s->hotbar[2] = s->blocks[BLOCK_PLANKS];
  s->hotbar[3] = s->blocks[BLOCK_COBBLESTONE];
  s->hotbar[4] = s->blocks[BLOCK_TILE];
  s->hotbar[5] = s->blocks[BLOCK_LOG];
  s->hotbar[6] = s->blocks[BLOCK_LEAVES];

  s->hotbar[7] = s->blocks[BLOCK_CLOTH_WHITE];
  s->hotbar[8] = s->blocks[BLOCK_CLOTH_RED];
  s->hotbar[9] = s->blocks[BLOCK_CLOTH_ORANGE];
  s->hotbar[10] = s->blocks[BLOCK_CLOTH_YELLOW];
  s->hotbar[11] = s->blocks[BLOCK_CLOTH_GREEN];
  s->hotbar[12] = s->blocks[BLOCK_CLOTH_BLUE];
  s->hotbar[13] = s->blocks[BLOCK_CLOTH_PURPLE];
  s->hotbar[14] = s->blocks[BLOCK_CLOTH_BLACK];

  if (!world_load())
  {
    world_generate();
  }
#endif
}

static void game_render(void);
bool game_tick(game_state_t *s)
{
  u64 frame_begin = sys_get_performance_counter();

  s_state = s;
  bool should_close = sys_should_close();
  if (should_close)
  {
    return false;
  }

  sys_poll_events();

#if STRIPPED_DOWN
  ren_frame_begin(wt_vec2(0, 0));

  ren_frame_end();
#else
  if (sys_key_pressed(SYS_KEYCODE_G))
  {
    world_generate();
  }

  job_tick();

  player_tick();

  i32 mouse_wheel = sys_mouse_get_wheel();
  if (mouse_wheel != 0)
  {
    s->hotbar_pos = (s->hotbar_pos - mouse_wheel) % INVENTORY_SIZE;
    // i thought % did this automatically...
    if (s->hotbar_pos < 0)
    {
      s->hotbar_pos = INVENTORY_SIZE - 1;
    }
  }

  if (sys_mouse_pressed(SYS_MOUSE_RIGHT))
  {
    world_raycast_t rc = world_raycast(4);
    if (rc.hit && !player_intersects_block_position(rc.neighbor))
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
    world_raycast_t rc = world_raycast(10000);
    if (rc.hit)
    {
      wt_vec3_t origin = rc.pos;
      const int radius = 50;

      wt_vec3_t begin = wt_vec3i_sub_i32(origin, radius);
      wt_vec3_t end = wt_vec3i_add_i32(origin, radius);

      for (int z = begin.z; z < end.z; ++z)
      {
        for (int y = begin.y; y < end.y; ++y)
        {
          for (int x = begin.x; x < end.x; ++x)
          {
            wt_vec3_t pos = wt_vec3(x, y, z);
            wt_vec3_t offset = wt_vec3i_sub(pos, origin);

            f32 dist_sq = (f32)(offset.x * offset.x) + (f32)(offset.y * offset.y) +
              (f32)(offset.z * offset.z);
            f32 target_dist_sq = (f32)radius * (f32)radius;

            if (world_within_bounds(pos) && dist_sq <= target_dist_sq)
            {
              world_set_block(pos, 0);
            }
          }
        }
      }
    }
  }

  if (sys_key_pressed(SYS_KEYCODE_R))
  {
    world_dbg_rebuild_meshes();
  }

  if (sys_key_pressed(SYS_KEYCODE_Q))
  {
    world_save();
  }

  if (sys_key_down(SYS_KEYCODE_ESCAPE))
  {
    return false;
  }

  game_render();

  u64 frame_end = sys_get_performance_counter();
  u64 perf_freq = sys_get_performance_frequency();

  WT_ASSERT(frame_end > frame_begin);
  s->delta_time = (f64)(frame_end - frame_begin) / (f64)perf_freq;
#endif
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

  world_render();

  wt_rect_t reticle[2] = {
    wt_rect(window_size.x / 2 - 1, window_size.y / 2 - 8, 2, 16),
    wt_rect(window_size.x / 2 - 8, window_size.y / 2 - 1, 16, 2),
  };
  ren_draw_rect(reticle[0], WT_COLOR_WHITE);
  ren_draw_rect(reticle[1], WT_COLOR_WHITE);

#if 0
  draw_debug_text(wt_vec2(1, 1), "camera pos:   (%f %f %f)",
    s->camera_pos.x, s->camera_pos.y, s->camera_pos.z);
  draw_debug_text(wt_vec2(1, 2), "camera front: (%f %f %f)",
    s->camera_front.x, s->camera_front.y, s->camera_front.z);
  draw_debug_text(wt_vec2(1, 3), "camera rotation: (%.2f %.2f)",
    s->camera_rotation.x * 360.0f, s->camera_rotation.y * 360.0f);
#endif

  wt_vec3f_t player_pos = player_get_position();
  draw_debug_text(wt_vec2(1, 1), "player position: (%.2f %.2f %.2f)",
    player_pos.x, player_pos.y, player_pos.z);

  block_info_t *holding_info = block_get_info(s->hotbar[s->hotbar_pos]);
  if (holding_info)
  {
    draw_debug_text(wt_vec2(1, 2), "holding: %s", holding_info->name);
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
