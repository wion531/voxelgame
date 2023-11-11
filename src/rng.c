#include "rng.h"
#include "game.h"
#include "memory.h"

typedef struct
{
  u64 state;
} rng_state_t;

static rng_state_t *get_state(void)
{
  return game_get_state()->modules.rng;
}

void rng_init(void)
{
  game_state_t *gs = game_get_state();
  rng_state_t *s = gs->modules.rng = mem_hunk_push(sizeof(rng_state_t));

  s->state = 0xb16b00b5b16b00b5;
}

void rng_mix(u64 x)
{
  rng_state_t *s = get_state();
  s->state ^= x;
}

u64 rng_next(void)
{
  rng_state_t *s = get_state();
  u64 x = s->state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  return s->state = x;
}

i64 rng_next_range(i64 low, i64 high)
{
  u64 x = rng_next();
  return (x % (high - low)) + low;
}

bool rng_next_bool(void)
{
  return rng_next() % 2 == 0;
}
