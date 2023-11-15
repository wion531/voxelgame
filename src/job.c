#include "job.h"
#include "game.h"
#include "memory.h"
#include "system.h"
#include <immintrin.h>

typedef struct
{
  job_func_t func;
  void *param;
} job_queue_entry_t;

typedef struct
{
  sys_mutex_t queue_mutex;
  job_queue_entry_t queue[JOB_QUEUE_SIZE];
  usize queue_pos;

  sys_thread_t *workers;
  usize num_workers;

  bool paused;
} job_state_t;

static job_state_t *get_state(void)
{
  game_state_t *gs = game_get_state();
  return gs->modules.job;
}

u32 job_thread_func(void *param)
{
  job_state_t *s = (job_state_t*)param;

  while (!s->paused)
  {
    if (s->queue_pos > 0)
    {
      job_queue_entry_t qe = { 0 };
      sys_mutex_lock(s->queue_mutex);
      if (s->queue_pos > 0)
      {
        qe = s->queue[--s->queue_pos];
      }
      sys_mutex_unlock(s->queue_mutex);

      if (qe.func)
      {
        qe.func(qe.param);
      }
    }
    // NOTE: this sleep makes chunk generation slower, but without it, a significant
    // amount of input lag is introduced.
    // when chunk generation is done in a cute little UI, see if this can be removed
    sys_thread_sleep(1);
    _mm_pause();
    //sys_thread_yield();
  }
  return 0;
}

void job_init(void)
{
  game_state_t *gs = game_get_state();
  job_state_t *s = gs->modules.job = mem_hunk_push(sizeof(job_state_t));

  s->queue_mutex = sys_mutex_new();

  s->num_workers = sys_cpu_get_num_cores() - 1;
  s->workers = mem_hunk_push(s->num_workers * sizeof(sys_thread_t));
  for (usize i = 0; i < s->num_workers; ++i)
  {
    s->workers[i] = sys_thread_new(job_thread_func, s);
  }
}

void job_queue(job_func_t func, void *param)
{
  job_state_t *s = get_state();
  job_resume_all();
  sys_mutex_lock(s->queue_mutex);
  s->queue[s->queue_pos++] = (job_queue_entry_t){ func, param };
  sys_mutex_unlock(s->queue_mutex);
}

void job_tick(void)
{
#if 0
  job_state_t *s = get_state();
  if (s->queue_pos == 0)
  {
    job_pause_all();
  }
#endif
}

usize job_get_num_workers(void)
{
  return get_state()->num_workers;
}

isize job_get_worker_id(void)
{
  job_state_t *s = get_state();
  u32 tid = sys_thread_get_current_id();
  for (isize i = 0; i < (isize)s->num_workers; ++i)
  {
    sys_thread_t hdl = s->workers[i];
    if (sys_thread_get_id(hdl) == tid)
    {
      return i;
    }
  }
  return -1;
}

void job_pause_all(void)
{
  job_state_t *s = get_state();
  if (!s) { return; } // could be called before initialization

  if (!s->paused)
  {
    s->paused = true;

    // wait for all threads to finish their work
    bool still_working = true;
    while (still_working)
    {
      still_working = false;
      for (usize i = 0; i < s->num_workers; ++i)
      {
        if (sys_thread_active(s->workers[i]))
        {
          still_working = true;
        }
      }

      sys_thread_sleep(1);
    }
  }
}

void job_resume_all(void)
{
  job_state_t *s = get_state();
  if (!s) { return; }

  if (s->paused)
  {
    s->paused = false;

    // start up threads again
    for (u32 i = 0; i < s->num_workers; ++i)
    {
      s->workers[i] = sys_thread_new(job_thread_func, s);
    }
  }
}
