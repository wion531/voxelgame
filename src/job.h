#ifndef JOB_H
#define JOB_H

#define JOB_QUEUE_SIZE 5000

#include <wt/wt.h>

typedef void (*job_func_t)(void *param);

void  job_init(void);
void  job_queue(job_func_t func, void *param);
void  job_tick(void);

usize job_get_num_workers(void);
isize job_get_worker_id(void);

void  job_pause_all(void);
void  job_resume_all(void);

#endif
