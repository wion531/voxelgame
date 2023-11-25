#include "player.h"
#include "game.h"
#include "memory.h"
#include "world.h"
#include "system.h"
#include <math.h>

#define WALK_SPEED 5.0f
#define RUN_SPEED 10.0f

#define GRAVITY -0.6f // sorry Mr. Davis

#define TERMINAL_VELOCITY -53.0f

#define MOVE_ACCEL 1.0f

#define JUMP_FORCE 10

typedef struct
{
  wt_vec3f_t position;
  wt_vec2f_t rotation;
  wt_vec3f_t velocity;

  wt_vec2f_t move_dir;

  f32 move_speed;

  bool running;
  bool on_ground;
  bool god_mode;
} player_state_t;

static player_state_t *get_state(void)
{
  return game_get_state()->modules.player;
}

void player_init(void)
{
  game_state_t *gs = game_get_state();
  player_state_t *s = gs->modules.player = mem_hunk_push(sizeof(player_state_t));

  s->position = wt_vec3f((WORLD_MAX_CHUNKS_X - 1) * CHUNK_SIZE_X, 511.0f,
    (WORLD_MAX_CHUNKS_Z - 1) * CHUNK_SIZE_Z);
}

// todo: this function and the next are extremely similar.
// find a way to refactor them
static bool player_collides_with_world(void)
{
  player_state_t *s = get_state();

  // worst case scenario is 12 blocks for a ~1x2x1 bounding box
  wt_vec3_t blocks[12] = { 0 };
  usize num_blocks = 0;

  // determine which blocks the player is intersecting
  i64 minx = floor(s->position.x - PLAYER_RADIUS);
  i64 maxx = ceil(s->position.x + PLAYER_RADIUS);
  i64 miny = floor(s->position.y);
  i64 maxy = ceil(s->position.y + PLAYER_HEIGHT);
  i64 minz = floor(s->position.z - PLAYER_RADIUS);
  i64 maxz = ceil(s->position.z + PLAYER_RADIUS);

  for (i64 y = miny; y < maxy; ++y)
  {
    for (i64 z = minz; z < maxz; ++z)
    {
      for (i64 x = minx; x < maxx; ++x)
      {
        blocks[num_blocks++] = wt_vec3(x, y, z);
      }
    }
  }

  // determine if the player bounding box intersects with any of them
  wt_vec3f_t player_size = wt_vec3f(PLAYER_RADIUS * 2, PLAYER_HEIGHT, PLAYER_RADIUS * 2);
  wt_vec3f_t player_pos = wt_vec3f(s->position.x - PLAYER_RADIUS,
    s->position.y, s->position.z - PLAYER_RADIUS); // bottom front left corner

  for (usize i = 0; i < num_blocks; ++i)
  {
    if (world_within_bounds(blocks[i]))
    {
      block_id_t id = world_get_block(blocks[i]);
      if (id != 0)
      {
        // todo: get mesh from block info
        const wt_vec3f_t block_size = wt_vec3f(1, 1, 1);
        wt_vec3f_t block_pos = wt_vec3i_to_vec3f(blocks[i]);

        bool collision =
          player_pos.x <= block_pos.x + block_size.x &&
          player_pos.x + player_size.x >= block_pos.x &&
          player_pos.y <= block_pos.y + block_size.y &&
          player_pos.y + player_size.y >= block_pos.y &&
          player_pos.z <= block_pos.z + block_size.z &&
          player_pos.z + player_size.z >= block_pos.z;
        if (collision)
        {
          return true;
        }
      }
    }
  }
  return false;
}

bool player_intersects_block_position(wt_vec3i_t pos)
{
  player_state_t *s = get_state();

  // worst case scenario is 12 blocks for a ~1x2x1 bounding box
  wt_vec3_t blocks[12] = { 0 };
  usize num_blocks = 0;

  // determine which blocks the player is intersecting
  i64 minx = floor(s->position.x - PLAYER_RADIUS);
  i64 maxx = ceil(s->position.x + PLAYER_RADIUS);
  i64 miny = floor(s->position.y);
  i64 maxy = ceil(s->position.y + PLAYER_HEIGHT);
  i64 minz = floor(s->position.z - PLAYER_RADIUS);
  i64 maxz = ceil(s->position.z + PLAYER_RADIUS);

  bool given_pos_within_bounds = false;
  for (i64 y = miny; y < maxy; ++y)
  {
    for (i64 z = minz; z < maxz; ++z)
    {
      for (i64 x = minx; x < maxx; ++x)
      {
        blocks[num_blocks++] = wt_vec3(x, y, z);
        if (pos.x == x && pos.y == y && pos.z == z)
        {
          given_pos_within_bounds = true;
        }
      }
    }
  }

  if (!given_pos_within_bounds)
  {
    return false;
  }

  // determine if the player bounding box intersects with any of them
  wt_vec3f_t player_size = wt_vec3f(PLAYER_RADIUS * 2, PLAYER_HEIGHT, PLAYER_RADIUS * 2);
  wt_vec3f_t player_pos = wt_vec3f(s->position.x - PLAYER_RADIUS,
    s->position.y, s->position.z - PLAYER_RADIUS); // bottom front left corner

  for (usize i = 0; i < num_blocks; ++i)
  {
    if (world_within_bounds(blocks[i]))
    {
      // todo: get mesh from block info
      const wt_vec3f_t block_size = wt_vec3f(1, 1, 1);
      wt_vec3f_t block_pos = wt_vec3i_to_vec3f(blocks[i]);

      bool collision =
        player_pos.x <= block_pos.x + block_size.x &&
        player_pos.x + player_size.x >= block_pos.x &&
        player_pos.y <= block_pos.y + block_size.y &&
        player_pos.y + player_size.y >= block_pos.y &&
        player_pos.z <= block_pos.z + block_size.z &&
        player_pos.z + player_size.z >= block_pos.z;
      if (collision)
      {
        return true;
      }
    }
  }
  return false;
}

void player_tick(void)
{
  game_state_t *gs = game_get_state();
  player_state_t *s = get_state();

  // === god mode toggle ===
  if (sys_key_pressed(SYS_KEYCODE_V))
  {
    s->god_mode = !s->god_mode;
  }

  // === mouse look ===
  wt_vec2_t mouse_delta = sys_mouse_get_delta();
  s->rotation.x += (f32)mouse_delta.x / 1000.0f;
  s->rotation.y -= (f32)mouse_delta.y / 1000.0f;
  s->rotation.y = WT_CLAMP(s->rotation.y, -0.24, +0.24);

  // === horizontal movement ===
  s->running = sys_key_down(SYS_KEYCODE_LSHIFT);

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

  bool moving = (dx != 0.0f || dz != 0.0f);
  f32 move_speed = (s->running) ? RUN_SPEED : WALK_SPEED;
  if (!moving)
  {
    move_speed = 0.0f;
  }

  if (s->move_speed < move_speed)
  {
    s->move_speed = WT_MIN(s->move_speed + MOVE_ACCEL, move_speed);
  }
  else if (s->move_speed > move_speed)
  {
    s->move_speed = WT_MAX(s->move_speed - MOVE_ACCEL, move_speed);
  }

  if (moving)
  {
    s->move_dir.x = dx;
    s->move_dir.y = dz;
  }

  f32 mag = sqrt(s->move_dir.x*s->move_dir.x + s->move_dir.y*s->move_dir.y);
  if (mag != 0.0f)
  {
    s->move_dir.x /= mag;
    s->move_dir.y /= mag;
  }

//  dx *= s->move_speed;
//  dz *= s->move_speed;

  s->velocity.x = s->move_dir.y * cosf(s->rotation.x * WT_TAU_F32);
  s->velocity.z = s->move_dir.y * sinf(s->rotation.x * WT_TAU_F32);
  s->velocity.x += s->move_dir.x * cosf((s->rotation.x + 0.25f) * WT_TAU_F32);
  s->velocity.z += s->move_dir.x * sinf((s->rotation.x + 0.25f) * WT_TAU_F32);

  s->velocity.x *= s->move_speed;
  s->velocity.z *= s->move_speed;

  // === falling velocity ===
  wt_vec3_t below_pos = { 0 };
  block_id_t below = 0;
  if (s->position.y <= CHUNK_SIZE_Y)
  {
    below_pos = wt_vec3f_to_vec3i(s->position);
    below_pos.y -= 1;
    below = world_get_block(below_pos);
    s->on_ground = (below != 0) && (s->position.y - floorf(s->position.y)) < 0.001f;
  }
  // no way is his ass on the ground
  else
  {
    s->on_ground = false;
  }

  if (!s->god_mode)
  {
    if (s->on_ground)
    {
      s->velocity.y = 0.0f;
    }
    else if (s->velocity.y > TERMINAL_VELOCITY)
    {
      s->velocity.y += GRAVITY; // TODO: use delta time
    }
  }

  // === jump ===
  if (sys_key_down(SYS_KEYCODE_SPACE) && (s->on_ground || s->god_mode))
  {
    s->on_ground = false;
    s->velocity.y = JUMP_FORCE;
  }
  else if (s->god_mode)
  {
    s->velocity.y = 0;
  }

  if (s->god_mode)
  {
    if (sys_key_down(SYS_KEYCODE_LCTRL))
    {
      s->velocity.y = -JUMP_FORCE;
    }

    s->velocity = wt_vec3f_mul_f32(s->velocity, 1.5f);
  }

  // === applying velocity ===
  wt_vec3f_t velocity_remaining = s->velocity;
  while (velocity_remaining.x != 0.0f || velocity_remaining.y != 0.0f || velocity_remaining.z != 0.0f)
  {
    f32 x = WT_CLAMP(velocity_remaining.x, -1.0f, 1.0f);
    f32 y = WT_CLAMP(velocity_remaining.y, -1.0f, 1.0f);
    f32 z = WT_CLAMP(velocity_remaining.z, -1.0f, 1.0f);

    // x advance
    wt_vec3f_t old_position = s->position;
    s->position.x += x * gs->delta_time;
    if (player_collides_with_world())
    {
      s->position = old_position;
      s->velocity.x = 0;
    }

    // y advance
#if 0
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
      s->position.y += y * gs->delta_time;
    }
    else
    {
      s->velocity.y = 0;
      if (y < 0.0f)
      {
        s->position.y = block_dest.y + 1;
      }
    }
#else
    old_position = s->position;
    s->position.y += y * gs->delta_time;
    if (player_collides_with_world())
    {
      if (y < 0.0f)
      {
        wt_vec3i_t block_dest = wt_vec3f_to_vec3i(s->position);
        s->position.y = block_dest.y + 1;
      }
      else
      {
        s->position.y = old_position.y;
      }

      s->velocity.y = 0;
    }
#endif

    // z advance
    old_position = s->position;
    s->position.z += z * gs->delta_time;
    if (player_collides_with_world())
    {
      s->position = old_position;
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
