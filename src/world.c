#include "world.h"
#include "memory.h"
#include "game.h"
#include "chunk.h"
#include "job.h"
#include "player.h"
#include <math.h>

typedef struct
{
  chunk_t *chunks[WORLD_MAX_CHUNKS];
} world_state_t;

world_state_t *get_state(void)
{
  return game_get_state()->modules.world;
}

void world_init(void)
{
  game_state_t *gs = game_get_state();
  world_state_t *s = gs->modules.world = mem_hunk_push(sizeof(world_state_t));

  for (usize i = 0; i < WORLD_MAX_CHUNKS; ++i)
  {
    s->chunks[i] = chunk_new(wt_vec2(i % WORLD_MAX_CHUNKS_X, i / WORLD_MAX_CHUNKS_X));
  }
}

void world_tick(void)
{
  world_state_t *s = get_state();
  // todo: do something
  WT_UNUSED(s);
}

void world_render(void)
{
  world_state_t *s = get_state();
  for (usize i = 0; i < WORLD_MAX_CHUNKS; ++i)
  {
    chunk_render(s->chunks[i]);
  }
}

static void chunk_gen_job(void *param)
{
  chunk_t *chunk = (chunk_t*)param;
  chunk_gen(chunk);
}

void world_generate(void)
{
  world_state_t *s = get_state();
  for (usize i = 0; i < WORLD_MAX_CHUNKS; ++i)
  {
    job_queue(chunk_gen_job, s->chunks[i]);
  }
}

void world_dbg_rebuild_meshes(void)
{
  world_state_t *s = get_state();
  for (u32 i = 0; i < WT_ARRAY_COUNT(s->chunks); ++i)
  {
    chunk_rebuild_mesh(s->chunks[i]);
  }
}

void world_set_block(wt_vec3_t pos, block_id_t block)
{
  world_state_t *s = get_state();

  wt_vec2_t chunk_pos = { 0 };
  chunk_pos.x = floorf(pos.x / CHUNK_SIZE_X);
  chunk_pos.y = floorf(pos.z / CHUNK_SIZE_Z);
  if (chunk_pos.x >= 0 && chunk_pos.y >= 0 &&
    chunk_pos.x < WORLD_MAX_CHUNKS_X && chunk_pos.y < WORLD_MAX_CHUNKS_Z)
  {
    chunk_t *c = s->chunks[chunk_pos.x + chunk_pos.y * WORLD_MAX_CHUNKS_X];

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
        s->chunks[(chunk_pos.x - 1) + chunk_pos.y * WORLD_MAX_CHUNKS_X]->dirty = true;
      }
      if (block_pos.x == CHUNK_SIZE_X - 1 && chunk_pos.x < WORLD_MAX_CHUNKS_X - 1)
      {
        s->chunks[(chunk_pos.x + 1) + chunk_pos.y * WORLD_MAX_CHUNKS_X]->dirty = true;
      }
      if (block_pos.z == 0 && chunk_pos.y > 0)
      {
        s->chunks[chunk_pos.x + (chunk_pos.y - 1) * WORLD_MAX_CHUNKS_X]->dirty = true;
      }
      if (block_pos.z == CHUNK_SIZE_X - 1 && chunk_pos.y < WORLD_MAX_CHUNKS_X - 1)
      {
        s->chunks[chunk_pos.x + (chunk_pos.y + 1) * WORLD_MAX_CHUNKS_X]->dirty = true;
      }
    }
  }
}

block_id_t world_get_block(wt_vec3_t pos)
{
  world_state_t *s = get_state();

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

  if (chunk_pos.x >= 0 && chunk_pos.y >= 0 &&
    chunk_pos.x < WORLD_MAX_CHUNKS_X && chunk_pos.y < WORLD_MAX_CHUNKS_Z)
  {  
    chunk_t *c = s->chunks[chunk_pos.x + chunk_pos.y * WORLD_MAX_CHUNKS_X];
    return chunk_get_block(c, block_pos);
  }
  return 0;
}

bool world_within_bounds(wt_vec3_t pos)
{
  return (pos.x >= 0 && pos.y >= 0 && pos.z >= 0 &&
    pos.x < WORLD_MAX_CHUNKS_X * CHUNK_SIZE_X &&
    pos.y < CHUNK_SIZE_Y &&
    pos.z < WORLD_MAX_CHUNKS_Z * CHUNK_SIZE_Z);
}

world_raycast_t world_raycast(int max_num_blocks)
{
  wt_vec3f_t player_pos = player_get_head_position();
  wt_vec2f_t player_rot = player_get_rotation();
  wt_vec3i_t map_pos = wt_vec3f_to_vec3i(player_pos);

  f32 yc = cosf(player_rot.y * WT_TAU_F32);
  f32 ys = sinf(player_rot.y * WT_TAU_F32);
  f32 ac = cosf(player_rot.x * WT_TAU_F32) * yc;
  f32 as = sinf(player_rot.x * WT_TAU_F32) * yc;

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
