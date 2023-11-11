#ifndef MEMORY_H
#define MEMORY_H

#include <wt/wt.h>

#define MEM_SCRATCH_DEPTH 256
#define MEM_WORKER_ARENA_SIZE WT_MEGABYTES(16)

void mem_init(void);
void mem_post_init(void); // this is seperate because we must wait for the system module to initialize

// use for allocations that will last the whole game
void *mem_hunk_push(usize num_bytes);

// use for allocations that only last within a scope
void mem_scratch_begin(void);
void *mem_scratch_push(usize num_bytes);
void mem_scratch_end(void);

#endif
