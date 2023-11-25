#ifndef WT_ALLOCATORS_H
#define WT_ALLOCATORS_H

typedef struct
{
  void *mem;
  usize size, pos;
} wt_arena_t;

typedef struct wt_pool_node_t wt_pool_node_t;
struct wt_pool_node_t
{
  wt_pool_node_t *next;
};

typedef struct
{
  void *mem;
  usize size, chunk_size;
  wt_pool_node_t *head;
} wt_pool_t;

typedef struct
{
  usize size;
  bool is_free;
} wt_buddy_block_t;

typedef struct
{
  wt_buddy_block_t *head;
  wt_buddy_block_t *tail;
  usize align;
} wt_buddy_t;

usize wt_align16(usize x);

wt_arena_t wt_arena_new(void *mem, usize size);
void      *wt_arena_push(wt_arena_t *self, usize size);
void       wt_arena_push_from(wt_arena_t *self, void *ptr, usize size);
void       wt_arena_pop(wt_arena_t *self, usize size);
void      *wt_arena_get_last(wt_arena_t *self, usize size);
void       wt_arena_clear(wt_arena_t *self);

wt_pool_t  wt_pool_new(void *mem, usize size_in_chunks, usize chunk_size);
void      *wt_pool_alloc(wt_pool_t *self);
void       wt_pool_free(wt_pool_t *self, void *item);
void       wt_pool_free_all(wt_pool_t *self);

wt_buddy_t wt_buddy_new(void *mem, usize capacity);
void      *wt_buddy_alloc(wt_buddy_t *b, usize size);
void       wt_buddy_release(wt_buddy_t *b, void *item);

#endif
