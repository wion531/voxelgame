#include "player.h"
#include "game.h"
#include "memory.h"
#include "world.h"
#include "system.h"
#include <math.h>

#define WALK_SPEED (5.0f / 60.0f)
#define RUN_SPEED (20.0f / 60.0f)

#define GRAVITY -0.6f // sorry Mr. Davis

#define TERMINAL_VELOCITY -53.0f

#define JUMP_FORCE 30

typedef struct
{
  wt_vec3f_t position;
  wt_vec2f_t rotation;
  wt_vec3f_t velocity;

  bool running;
  bool on_ground;
} player_state_t;

static player_state_t *get_state(void)
{
  return game_get_state()->modules.player;
}

void player_init(void)
{
  game_state_t *gs = game_get_state();
  player_state_t *s = gs->modules.player = mem_hunk_push(sizeof(player_state_t));

  s->position = wt_vec3f(1023.0f, 511.0f, 1023.0f);
}

void player_tick(void)
{
  player_state_t *s = get_state();

  // === mouse look ===
  wt_vec2_t mouse_delta = sys_mouse_get_delta();
  s->rotation.x += (f32)mouse_delta.x / 1000.0f;
  s->rotation.y -= (f32)mouse_delta.y / 1000.0f;
  s->rotation.y = WT_CLAMP(s->rotation.y, -0.24, +0.24);

  // === horizontal movement ===
  s->running = sys_key_down(SYS_KEYCODE_LSHIFT);
  f32 move_speed = (s->running) ? RUN_SPEED : WALK_SPEED;
  
  f32 dx = 0.0f;
  f32 dz = 0.0f;

  if (sys_key_down(SYS_KEYCODE_W))
  {
    dz = 1.0f;
  }
  if (sys_key_down(SYS_KEYCODE_S))
  {
    dz = -1.0f;
  }

  if (sys_key_down(SYS_KEYCODE_A))
  {
    dx = -1.0f;
  }
  if (sys_key_down(SYS_KEYCODE_D))
  {
    dx = 1.0f;
  }

  f32 mag = sqrt(dx*dx + dz*dz);
  if (mag != 0.0f)
  {
    dx /= mag;
    dz /= mag;
  }

  dx *= move_speed;
  dz *= move_speed;

  s->velocity.x = dz * cosf(s->rotation.x * WT_TAU_F32);
  s->velocity.z = dz * sinf(s->rotation.x * WT_TAU_F32);

  s->velocity.x += dx * cosf((s->rotation.x + 0.25f) * WT_TAU_F32);
  s->velocity.z += dx * sinf((s->rotation.x + 0.25f) * WT_TAU_F32);

  // === falling velocity ===
  wt_vec3_t below_pos = { 0 };
  block_id_t below = 0;
  if (s->position.y <= CHUNK_SIZE_Y)
  {
    below_pos = wt_vec3f_to_vec3i(s->position);
    below_pos.y -= 1;
    below = world_get_block(below_pos);
    s->on_ground = (below != 0) && (s->position.y - floorf(s->position.y)) == 0.0f;
  }
  // no way is his ass on the ground
  else
  {
    s->on_ground = false;
  }

  if (s->on_ground)
  {
    s->velocity.y = 0.0f;
  }
  else if (s->velocity.y > TERMINAL_VELOCITY)
  {
    s->velocity.y += GRAVITY / 60.0f; // TODO: use delta time
  }

  // === jump ===
  if (sys_key_down(SYS_KEYCODE_SPACE) && s->on_ground)
  {
    s->on_ground = false;
    s->velocity.y = JUMP_FORCE / 60.0f;
  }

  // === applying velocity ===
  wt_vec3f_t velocity_remaining = s->velocity;
  while (velocity_remaining.x != 0.0f || velocity_remaining.y != 0.0f || velocity_remaining.z != 0.0f)
  {
    f32 x = WT_CLAMP(velocity_remaining.x, -1.0f, 1.0f);
    f32 y = WT_CLAMP(velocity_remaining.y, -1.0f, 1.0f);
    f32 z = WT_CLAMP(velocity_remaining.z, -1.0f, 1.0f);

    // x advance
    wt_vec3f_t dest = wt_vec3f_add(s->position, wt_vec3f(x, 0, 0));
    if (x > 0.0f) dest.x += PLAYER_RADIUS; else dest.x -= PLAYER_RADIUS;
    
    wt_vec3i_t block_dest = wt_vec3f_to_vec3i(dest);
    block_id_t block = 0;
    if (world_within_bounds(block_dest))
    {
      block = world_get_block(block_dest);
    }
    if (block == 0)
    {
      s->position.x += x;
    }
    else
    {
      s->velocity.x = 0;
    }

    // y advance
    dest = wt_vec3f_add(s->position, wt_vec3f(0, y, 0));
    if (y > 0.0f) { dest.y += PLAYER_HEIGHT; }
    
    block_dest = wt_vec3f_to_vec3i(dest);
    block = 0;
    if (world_within_bounds(block_dest))
    {
      block = world_get_block(block_dest);
    }
    if (block == 0)
    {
      s->position.y += y;
    }
    else
    {
      s->velocity.y = 0;
      if (y < 0.0f)
      {
        s->position.y = block_dest.y + 1;
      }
    }

    // z advance
    dest = wt_vec3f_add(s->position, wt_vec3f(0, 0, z));
    if (z > 0.0f) dest.z += PLAYER_RADIUS; else dest.z -= PLAYER_RADIUS;
    
    block_dest = wt_vec3f_to_vec3i(dest);
    block = 0;
    if (world_within_bounds(block_dest))
    {
      block = world_get_block(block_dest);
    }
    if (block == 0)
    {
      s->position.z += z;
    }
    else
    {
      s->velocity.z = 0;
    }

    velocity_remaining.x -= x;
    velocity_remaining.y -= y;
    velocity_remaining.z -= z;
  }

  // === camera projection ===
  wt_vec3f_t camera_pos = wt_vec3f_div_f32(s->position, 2.0f); // i still don't know why it's half...
  camera_pos.y += PLAYER_HEIGHT / 2.0f; // the camera should be at the head

  static const wt_vec3f_t k_camera_up = { 0, 1, 0 };
 
  wt_vec3f_t camera_front = { 0, 0, 1 };

  wt_vec3f_t front = { 0 };
  front.x = cosf(s->rotation.x * WT_TAU_F32) * cosf(s->rotation.y * WT_TAU_F32);
  front.y = sinf(s->rotation.y * WT_TAU_F32);
  front.z = sinf(s->rotation.x * WT_TAU_F32) * cosf(s->rotation.y * WT_TAU_F32);
  camera_front = wt_vec3f_norm(front);

  wt_mat4x4_t direction = wt_mat4x4_look_at(camera_pos,
    wt_vec3f_add(camera_pos, camera_front), k_camera_up);

  ren_camera_set(direction);

  if (sys_key_pressed(SYS_KEYCODE_P))
  {
    s->position = wt_vec3f((WORLD_MAX_CHUNKS_X - 1) * CHUNK_SIZE_X, 511.0f,
      (WORLD_MAX_CHUNKS_Z - 1) * CHUNK_SIZE_Z);
  }
}

wt_vec3f_t player_get_position(void)
{
  player_state_t *s = get_state();
  return s->position;
}

wt_vec2f_t player_get_rotation(void)
{
  player_state_t *s = get_state();
  return s->rotation;
}

wt_vec3f_t player_get_head_position(void)
{
  player_state_t *s = get_state();
  return wt_vec3f_add(s->position, wt_vec3f(0, PLAYER_HEIGHT, 0));
}
