#include "chunk.h"
#include "game.h"
#include "memory.h"
#include "job.h"
#include "world.h"
#include <math.h>

typedef struct
{
  wt_pool_t pool;
} chunk_state_t;

static chunk_state_t *get_state(void)
{
  return game_get_state()->modules.chunk;
}

void chunk_init(void)
{
  game_state_t *gs = game_get_state();
  chunk_state_t *s = gs->modules.chunk = mem_hunk_push(sizeof(chunk_state_t));

  usize pool_size = ((sizeof(chunk_t) + 0xf) & ~0xf) * (CHUNK_MAX + 2);
  s->pool = wt_pool_new(mem_hunk_push(pool_size), CHUNK_MAX + 2, sizeof(chunk_t));
}

chunk_t *chunk_new(wt_vec2_t pos)
{
  chunk_state_t *s = get_state();
  chunk_t *res = wt_pool_alloc(&s->pool);
  if (res)
  {
    res->mesh = ren_chunk_new(pos);
    res->position = pos;
  }
  return res;
}

static f32 interpolate(f32 x, f32 y, f32 w)
{
  return (y - x) * w + x;
}

static f32 smoothstep(f32 x, f32 y, f32 w)
{
  f32 v = w * w * w * (w * (w * 6 - 15) + 10);
  return x + v * (y - x);
}

static u32 scramble(u32 a)
{
#if 0
  a = (a+0x7ed55d16) + (a<<12);
  a = (a^0xc761c23c) ^ (a>>19);
  a = (a+0x165667b1) + (a<<5);
  a = (a+0xd3a2646c) ^ (a<<9);
  a = (a+0xfd7046c5) + (a<<3);
  a = (a^0xb55a4f09) ^ (a>>16);
#elif 0
  a ^= (a >> 17);
  a *= 0xed5ad4bb;
  a ^= (a >> 11);
  a *= 0xac4c1b51;
  a ^= (a >> 15);
  a *= 0x31848bab;
  a ^= (a >> 14);
#else
  a ^= a >> 16;
  a *= 0x7feb352dU;
  a ^= a >> 15;
  a *= 0x846ca68bU;
  a ^= a >> 16;
#endif

  return a;
}

static const f32 k_sin_table[256] = {
  +0.0000, +0.0245, +0.0491, +0.0736, +0.0980, +0.1224, +0.1467, +0.1710,
  +0.1951, +0.2191, +0.2430, +0.2667, +0.2903, +0.3137, +0.3369, +0.3599,
  +0.3827, +0.4052, +0.4276, +0.4496, +0.4714, +0.4929, +0.5141, +0.5350,
  +0.5556, +0.5758, +0.5957, +0.6152, +0.6344, +0.6532, +0.6716, +0.6895,
  +0.7071, +0.7242, +0.7410, +0.7572, +0.7730, +0.7883, +0.8032, +0.8176,
  +0.8315, +0.8449, +0.8577, +0.8701, +0.8819, +0.8932, +0.9040, +0.9142,
  +0.9239, +0.9330, +0.9415, +0.9495, +0.9569, +0.9638, +0.9700, +0.9757,
  +0.9808, +0.9853, +0.9892, +0.9925, +0.9952, +0.9973, +0.9988, +0.9997,
  +1.0000, +0.9997, +0.9988, +0.9973, +0.9952, +0.9925, +0.9892, +0.9853,
  +0.9808, +0.9757, +0.9700, +0.9638, +0.9569, +0.9495, +0.9415, +0.9330,
  +0.9239, +0.9142, +0.9040, +0.8932, +0.8819, +0.8701, +0.8577, +0.8449,
  +0.8315, +0.8176, +0.8032, +0.7883, +0.7730, +0.7572, +0.7410, +0.7242,
  +0.7071, +0.6895, +0.6716, +0.6532, +0.6344, +0.6152, +0.5957, +0.5758,
  +0.5556, +0.5350, +0.5141, +0.4929, +0.4714, +0.4496, +0.4276, +0.4052,
  +0.3827, +0.3599, +0.3369, +0.3137, +0.2903, +0.2667, +0.2430, +0.2191,
  +0.1951, +0.1710, +0.1467, +0.1224, +0.0980, +0.0736, +0.0491, +0.0245,
  +0.0000, -0.0245, -0.0491, -0.0736, -0.0980, -0.1224, -0.1467, -0.1710,
  -0.1951, -0.2191, -0.2430, -0.2667, -0.2903, -0.3137, -0.3369, -0.3599,
  -0.3827, -0.4052, -0.4276, -0.4496, -0.4714, -0.4929, -0.5141, -0.5350,
  -0.5556, -0.5758, -0.5957, -0.6152, -0.6344, -0.6532, -0.6716, -0.6895,
  -0.7071, -0.7242, -0.7410, -0.7572, -0.7730, -0.7883, -0.8032, -0.8176,
  -0.8315, -0.8449, -0.8577, -0.8701, -0.8819, -0.8932, -0.9040, -0.9142,
  -0.9239, -0.9330, -0.9415, -0.9495, -0.9569, -0.9638, -0.9700, -0.9757,
  -0.9808, -0.9853, -0.9892, -0.9925, -0.9952, -0.9973, -0.9988, -0.9997,
  -1.0000, -0.9997, -0.9988, -0.9973, -0.9952, -0.9925, -0.9892, -0.9853,
  -0.9808, -0.9757, -0.9700, -0.9638, -0.9569, -0.9495, -0.9415, -0.9330,
  -0.9239, -0.9142, -0.9040, -0.8932, -0.8819, -0.8701, -0.8577, -0.8449,
  -0.8315, -0.8176, -0.8032, -0.7883, -0.7730, -0.7572, -0.7410, -0.7242,
  -0.7071, -0.6895, -0.6716, -0.6532, -0.6344, -0.6152, -0.5957, -0.5758,
  -0.5556, -0.5350, -0.5141, -0.4929, -0.4714, -0.4496, -0.4276, -0.4052,
  -0.3827, -0.3599, -0.3369, -0.3137, -0.2903, -0.2667, -0.2430, -0.2191,
  -0.1951, -0.1710, -0.1467, -0.1224, -0.0980, -0.0736, -0.0491, -0.0245,
};

static f32 stupid_sin(u32 x)
{
  return k_sin_table[x & 0xff];
}

static u32 random_from_position(int x, int y)
{
  return scramble(scramble(x ^ ~(x << (y & 0x3)) * y));
}

static wt_vec2f_t random_gradient(int ix, int iy)
{
#if 0
  const u32 w = 8 * sizeof(u32);
  const u32 s = w / 2; // rotation width
  u32 a = ix, b = iy;
  a *= 3284157443; b ^= a << s | a >> (w-s);
  b *= 1911520717; a ^= b << s | b >> (w-s);
  a *= 2048419325;
  f32 random = a * (WT_PI_F32 / ~(~0u >> 1)); // in [0, 2*Pi]
  return wt_vec2f(cos(random), sin(random));
#else
  u32 sc = random_from_position(ix, iy);

  f32 s = stupid_sin(sc);
  f32 c = stupid_sin(64 - sc);

  return wt_vec2f(c, s);
#endif
}

static f32 dot_grid_gradient(int ix, int iy, f32 x, f32 y)
{
  wt_vec2f_t gradient = random_gradient(ix, iy);

  f32 dx = x - (f32)ix;
  f32 dy = y - (f32)iy;

  return (dx * gradient.x + dy * gradient.y);
}

static f32 perlin(f32 x, f32 y)
{
  int x0 = (int)floor(x);
  int x1 = x0 + 1;
  int y0 = (int)floor(y);
  int y1 = y0 + 1;

  f32 sx = x - (f32)x0;
  f32 sy = y - (f32)y0;

  f32 n0, n1, ix0, ix1, val;
  n0 = dot_grid_gradient(x0, y0, x, y);
  n1 = dot_grid_gradient(x1, y0, x, y);
  ix0 = smoothstep(n0, n1, sx);

  n0 = dot_grid_gradient(x0, y1, x, y);
  n1 = dot_grid_gradient(x1, y1, x, y);
  ix1 = smoothstep(n0, n1, sx);

  val = smoothstep(ix0, ix1, sy);
  return val;
}

#define PERLIN_NUM_STEPS 4
static block_id_t get_block(wt_vec3_t pos)
{
  game_state_t *s = game_get_state();
  if (pos.y < 200)
  {
    f32 p = 0;
#if 1
    for (int i = 0; i < PERLIN_NUM_STEPS; ++i)
    {
      f32 m = 0.015f * (PERLIN_NUM_STEPS - i);
      f32 s = perlin(pos.x * m, pos.z * m);
      s = (s + 1) / 2.0f;
      f32 lo = i * (1.0f / (PERLIN_NUM_STEPS + 1));
      f32 hi = 1.4f - lo;
      p *= interpolate(lo, hi, s);
      p += 1.0f;
    }

    p *= 50.0f;

//    p = stupid_sin((f32)pos.x * 10.0f) * 10;

    p += 10;
#else
    p = 20;
#endif

    bool solid = pos.y <= p;
    bool grass = pos.y == (int)p;

    if (solid)
    {
      return s->blocks[grass ? BLOCK_GRASS_BLOCK : BLOCK_DIRT];
    }
  }
  return 0; // air
}

static void place_tree(wt_vec2_t pos)
{
  int x = pos.x, z = pos.y;
  int y = 0;

  // find the block right above the ground
  for (y = CHUNK_SIZE_Y - 1; y >= 0; --y)
  {
    block_id_t block = world_get_block(wt_vec3(x, y, z));
    if (block != 0)
    {
      y += 1;

      // we don't want trees on trees
      if (block == BLOCK_LEAVES)
      {
        return;
      }

      break;
    }
  }

  // build a stump of random height
  u32 height = ((random_from_position(pos.x, pos.x) ^ scramble(pos.y)) & 0x3) + 6;
  for (u32 i = 0; i < height; ++i)
  {
    wt_vec3_t log_pos = wt_vec3(x, y + i, z);
    if (world_within_bounds(log_pos))
    {
      world_set_block(log_pos, BLOCK_LOG);
    }
  }

  // make a sphere of leaves
  wt_vec3_t leaves_center = wt_vec3(x, y + height, z);

  i32 radius = 4;
  for (int yi = leaves_center.y - radius; yi < leaves_center.y + radius; ++yi)
  {
    for (int zi = leaves_center.z - radius; zi < leaves_center.z + radius; ++zi)
    {
      for (int xi = leaves_center.x - radius; xi < leaves_center.x + radius; ++xi)
      {
        wt_vec3_t leaf_pos = wt_vec3(xi, yi, zi);
        wt_vec3_t offset = wt_vec3i_sub(leaf_pos, leaves_center);

        f32 dist_sq = (f32)(offset.x * offset.x) + (f32)(offset.y * offset.y) +
          (f32)(offset.z * offset.z);
        f32 target_dist_sq = (f32)radius * (f32)radius;

        if (world_within_bounds(leaf_pos) && dist_sq <= target_dist_sq &&
          (xi != leaves_center.x || zi != leaves_center.z || yi > leaves_center.y))
        {
          world_set_block(leaf_pos, BLOCK_LEAVES);
        }
      }
    }
  }
}

void chunk_gen(chunk_t *c)
{
  // terrain shape
  for (usize i = 0; i < CHUNK_NUM_BLOCKS; ++i)
  {
    wt_vec3_t pos = { 0 };
    pos.x = i % CHUNK_SIZE_X;
    pos.z = (i / CHUNK_SIZE_X) % (CHUNK_SIZE_Z);
    pos.y = i / (CHUNK_SIZE_X * CHUNK_SIZE_Z);

    pos = wt_vec3i_add(pos, wt_vec3i_mul_i32(wt_vec3(c->position.x, 0, c->position.y), 16));

    c->blocks[i] = get_block(pos);
  }

  // structures
  for (usize z = 0; z < CHUNK_SIZE_Z; ++z)
  {
    for (usize x = 0; x < CHUNK_SIZE_X; ++x)
    {
      usize wx = c->position.x * CHUNK_SIZE_X + x;
      usize wz = c->position.y * CHUNK_SIZE_Z + z;

      u32 random = random_from_position(wx, wz);
      if ((random & 0xff) == 0)
      {
        place_tree(wt_vec2(wx, wz));
      }
    }
  }

  c->dirty = true;
}

void chunk_set_block(chunk_t *c, wt_vec3_t position, block_id_t block)
{
  usize offset = position.x + (position.z * CHUNK_SIZE_Z) + (position.y * CHUNK_SIZE_X * CHUNK_SIZE_Z);
  WT_ASSERT(offset < CHUNK_NUM_BLOCKS);
  c->blocks[offset] = block;
  c->dirty = true;
}

block_id_t chunk_get_block(chunk_t *c, wt_vec3_t position)
{
  usize offset = position.x + (position.z * CHUNK_SIZE_Z) + (position.y * CHUNK_SIZE_X * CHUNK_SIZE_Z);
  WT_ASSERT(offset < CHUNK_NUM_BLOCKS);
  return c->blocks[offset];
}

void rebuild_job(void *param)
{
  chunk_t *c = (chunk_t*)param;
  ren_chunk_generate_mesh(c->mesh, c->blocks);
}

void chunk_rebuild_mesh(chunk_t *c)
{
  job_queue(rebuild_job, c);
}

void chunk_render(chunk_t *c)
{
  if (c->dirty)
  {
    chunk_rebuild_mesh(c);
    c->dirty = false;
  }

  ren_draw_chunk(c->mesh);
}

void chunk_free(chunk_t *c)
{
  chunk_state_t *s = get_state();
  ren_chunk_free(c->mesh);
  wt_pool_free(&s->pool, c);
}
