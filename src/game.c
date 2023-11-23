#include "game.h"
#include "memory.h"
#include "system.h"
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

game_state_t *s_state;

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
    world_generate();
  }

  job_tick();

  // === camera movement ===
#if 0
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
#else
  player_tick();
#endif

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
