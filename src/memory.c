#include "memory.h"
#include "game.h"
#include "constants.h"
#include "system.h"
#include "job.h"
#include <string.h>

typedef struct
{
  void *hunk;

  usize bottom, top;
  usize scratch_stack[MEM_SCRATCH_DEPTH];
  usize scratch_stack_size;

  wt_arena_t *worker_arenas;
  u32 *worker_arenas_scratch;
  u32 *worker_arenas_pos;
} mem_state_t;

static mem_state_t *get_state(void)
{
  return game_get_state()->modules.mem;
}

void mem_init(void)
{
  game_state_t *gs = game_get_state();
  mem_state_t *s = (mem_state_t*)((byte_t*)gs->hunk + sizeof(game_state_t));
  gs->modules.mem = s;

  s->hunk = gs->hunk;
  s->bottom = sizeof(game_state_t) + sizeof(mem_state_t);
}

void mem_post_init(void)
{
  mem_state_t *s = get_state();

  usize num_workers = job_get_num_workers();
  s->worker_arenas = mem_hunk_push(sizeof(wt_arena_t) * num_workers);
  s->worker_arenas_scratch = mem_hunk_push(sizeof(u32) * num_workers * MEM_SCRATCH_DEPTH);
  s->worker_arenas_pos = mem_hunk_push(sizeof(u32) * num_workers);

  for (usize i = 0; i < num_workers; ++i)
  {
    s->worker_arenas[i] = wt_arena_new(mem_hunk_push(MEM_WORKER_ARENA_SIZE), MEM_WORKER_ARENA_SIZE);
  }
}

void *mem_hunk_push(usize num_bytes)
{
  mem_state_t *s = get_state();

  usize aligned = (num_bytes + 0xf) & ~0xf;
  if (s->bottom + aligned < HUNK_SIZE - s->top)
  {
    void *res = (byte_t*)s->hunk + s->bottom;
    s->bottom += aligned;
    return memset(res, 0, aligned);
  }
  WT_ASSERT(false && "allocation failed!");
  return NULL;
}

void mem_scratch_begin(void)
{
  mem_state_t *s = get_state();
  isize worker_id = job_get_worker_id();
  if (worker_id == -1)
  {
    usize pos = s->top;
    if (s->scratch_stack_size + 1 < MEM_SCRATCH_DEPTH)
    {
      s->scratch_stack[s->scratch_stack_size++] = pos;
    }
    else
    {
      WT_ASSERT(false && "scratch depth exceeded");
    }
  }
  else
  {
    u32 *pos = &s->worker_arenas_pos[worker_id];
    u32 *stack = &s->worker_arenas_scratch[worker_id * MEM_SCRATCH_DEPTH];

    WT_ASSERT(*pos + 1 < MEM_SCRATCH_DEPTH);

    stack[*pos] = s->worker_arenas[worker_id].pos;
    *pos += 1;
  }
}

void *mem_scratch_push(usize num_bytes)
{
  mem_state_t *s = get_state();
  isize worker_id = job_get_worker_id();
  if (worker_id == -1)
  {
    usize aligned = (num_bytes + 0xf) & ~0xf;
    if (HUNK_SIZE - s->top - aligned > s->bottom)
    {
      void *res = (byte_t*)s->hunk + HUNK_SIZE - s->top - aligned;
      s->top += aligned;
      return memset(res, 0, aligned);
    }
    WT_ASSERT(false && "allocation failed!");
    return NULL;
  }
  else
  {
    wt_arena_t *arena = &s->worker_arenas[worker_id];
    return wt_arena_push(arena, num_bytes);
  }
}

void mem_scratch_end(void)
{
  mem_state_t *s = get_state();
  isize worker_id = job_get_worker_id();
  if (worker_id == -1)
  {
    if (s->scratch_stack_size > 0)
    {
      usize pos = s->scratch_stack[--s->scratch_stack_size];
      s->top = pos;
    }
    else
    {
      WT_ASSERT(false && "scratch stack underflow");
    }
  }
  else
  {
    u32 *pos = &s->worker_arenas_pos[worker_id];
    u32 *stack = &s->worker_arenas_scratch[worker_id * MEM_SCRATCH_DEPTH];

    WT_ASSERT(*pos > 0);

    s->worker_arenas[worker_id].pos = stack[*pos - 1];
    *pos -= 1;
  }
}
