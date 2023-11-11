#include "chunk.h"
#include "game.h"
#include "memory.h"
#include "job.h"
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

static u32 scramble(u32 a)
{
  a = (a+0x7ed55d16) + (a<<12);
  a = (a^0xc761c23c) ^ (a>>19);
  a = (a+0x165667b1) + (a<<5);
  a = (a+0xd3a2646c) ^ (a<<9);
  a = (a+0xfd7046c5) + (a<<3);
  a = (a^0xb55a4f09) ^ (a>>16);

  a = (a+0x7ed55d16) + (a<<12);
  a = (a^0xc761c23c) ^ (a>>19);
  a = (a+0x165667b1) + (a<<5);
  a = (a+0xd3a2646c) ^ (a<<9);
  a = (a+0xfd7046c5) + (a<<3);
  a = (a^0xb55a4f09) ^ (a>>16);
  
  return a;
}

static const f32 k_sin_table[256] = {
  0.0000, 0.0245, 0.0491, 0.0736, 0.0980, 0.1224, 0.1467, 0.1710,
  0.1951, 0.2191, 0.2430, 0.2667, 0.2903, 0.3137, 0.3369, 0.3599,
  0.3827, 0.4052, 0.4276, 0.4496, 0.4714, 0.4929, 0.5141, 0.5350,
  0.5556, 0.5758, 0.5957, 0.6152, 0.6344, 0.6532, 0.6716, 0.6895,
  0.7071, 0.7242, 0.7410, 0.7572, 0.7730, 0.7883, 0.8032, 0.8176,
  0.8315, 0.8449, 0.8577, 0.8701, 0.8819, 0.8932, 0.9040, 0.9142,
  0.9239, 0.9330, 0.9415, 0.9495, 0.9569, 0.9638, 0.9700, 0.9757,
  0.9808, 0.9853, 0.9892, 0.9925, 0.9952, 0.9973, 0.9988, 0.9997,
  1.0000, 0.9997, 0.9988, 0.9973, 0.9952, 0.9925, 0.9892, 0.9853,
  0.9808, 0.9757, 0.9700, 0.9638, 0.9569, 0.9495, 0.9415, 0.9330,
  0.9239, 0.9142, 0.9040, 0.8932, 0.8819, 0.8701, 0.8577, 0.8449,
  0.8315, 0.8176, 0.8032, 0.7883, 0.7730, 0.7572, 0.7410, 0.7242,
  0.7071, 0.6895, 0.6716, 0.6532, 0.6344, 0.6152, 0.5957, 0.5758,
  0.5556, 0.5350, 0.5141, 0.4929, 0.4714, 0.4496, 0.4276, 0.4052,
  0.3827, 0.3599, 0.3369, 0.3137, 0.2903, 0.2667, 0.2430, 0.2191,
  0.1951, 0.1710, 0.1467, 0.1224, 0.0980, 0.0736, 0.0491, 0.0245,
  0.0000, 0.0245, 0.0491, 0.0736, 0.0980, 0.1224, 0.1467, 0.1710,
  0.1951, 0.2191, 0.2430, 0.2667, 0.2903, 0.3137, 0.3369, 0.3599,
  0.3827, 0.4052, 0.4276, 0.4496, 0.4714, 0.4929, 0.5141, 0.5350,
  0.5556, 0.5758, 0.5957, 0.6152, 0.6344, 0.6532, 0.6716, 0.6895,
  0.7071, 0.7242, 0.7410, 0.7572, 0.7730, 0.7883, 0.8032, 0.8176,
  0.8315, 0.8449, 0.8577, 0.8701, 0.8819, 0.8932, 0.9040, 0.9142,
  0.9239, 0.9330, 0.9415, 0.9495, 0.9569, 0.9638, 0.9700, 0.9757,
  0.9808, 0.9853, 0.9892, 0.9925, 0.9952, 0.9973, 0.9988, 0.9997,
  1.0000, 0.9997, 0.9988, 0.9973, 0.9952, 0.9925, 0.9892, 0.9853,
  0.9808, 0.9757, 0.9700, 0.9638, 0.9569, 0.9495, 0.9415, 0.9330,
  0.9239, 0.9142, 0.9040, 0.8932, 0.8819, 0.8701, 0.8577, 0.8449,
  0.8315, 0.8176, 0.8032, 0.7883, 0.7730, 0.7572, 0.7410, 0.7242,
  0.7071, 0.6895, 0.6716, 0.6532, 0.6344, 0.6152, 0.5957, 0.5758,
  0.5556, 0.5350, 0.5141, 0.4929, 0.4714, 0.4496, 0.4276, 0.4052,
  0.3827, 0.3599, 0.3369, 0.3137, 0.2903, 0.2667, 0.2430, 0.2191,
  0.1951, 0.1710, 0.1467, 0.1224, 0.0980, 0.0736, 0.0491, 0.0245,
};

static f32 stupid_sin(u32 x)
{
  return k_sin_table[x & 0xff];
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
  u32 sc = scramble((ix ^ ~(ix << (iy & 0x3)) * iy));

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
  ix0 = interpolate(n0, n1, sx);

  n0 = dot_grid_gradient(x0, y1, x, y);
  n1 = dot_grid_gradient(x1, y1, x, y);
  ix1 = interpolate(n0, n1, sx);

  val = interpolate(ix0, ix1, sy);
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
      f32 m = 0.012f * (PERLIN_NUM_STEPS - i);
      f32 s = perlin(pos.x * m, pos.z * m);
      s = (s + 1) / 2.0f;
      f32 lo = i * (1.0f / (PERLIN_NUM_STEPS + 1));
      f32 hi = 1.5f - lo;
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

void chunk_gen(chunk_t *c)
{
  for (usize i = 0; i < CHUNK_NUM_BLOCKS; ++i)
  {
    wt_vec3_t pos = { 0 };
    pos.x = i % CHUNK_SIZE_X;
    pos.z = (i / CHUNK_SIZE_X) % (CHUNK_SIZE_Z);
    pos.y = i / (CHUNK_SIZE_X * CHUNK_SIZE_Z);

    pos = wt_vec3i_add(pos, wt_vec3i_mul_i32(wt_vec3(c->position.x, 0, c->position.y), 16));
    
    c->blocks[i] = get_block(pos);
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
