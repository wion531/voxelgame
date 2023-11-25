#include <wt/wt.h>
#include <string.h>

usize wt_align16(usize x)
{
  return (x + 0xf) & ~0xf;
}

wt_arena_t wt_arena_new(void *mem, usize size)
{
  wt_arena_t res = {0};
  res.mem = mem;
  res.size = size;
  return res;
}

void *wt_arena_push(wt_arena_t *self, usize size)
{
  size = wt_align16(size);
  void *res = (byte_t *)self->mem + self->pos;
  if (self->pos + size < self->size)
  {
    self->pos += size;
    memset(res, 0, size);
    return res;
  }
  return NULL;
}

void wt_arena_push_from(wt_arena_t *self, void *ptr, usize size)
{
  void *dst = wt_arena_push(self, size);
  memcpy(dst, ptr, size);
}

void wt_arena_pop(wt_arena_t *self, usize size)
{
  size = wt_align16(size);
  if (size <= self->pos)
  {
    self->pos -= size;
  }
}

void *wt_arena_get_last(wt_arena_t *self, usize size)
{
  size = wt_align16(size);
  return (byte_t*)self->mem + self->pos - size;
}

void wt_arena_clear(wt_arena_t *self)
{
  self->pos = 0;
}

wt_pool_t wt_pool_new(void *mem, usize size_in_chunks, usize chunk_size)
{
  wt_pool_t res = {0};
  res.mem = mem;
  res.size = size_in_chunks * chunk_size;
  res.chunk_size = wt_align16(chunk_size);
  wt_pool_free_all(&res);
  return res;
}

void *wt_pool_alloc(wt_pool_t *self)
{
  wt_pool_node_t *node = self->head;
  if (node == NULL)
  {
    return NULL;
  }
  self->head = self->head->next;
  memset(node, 0, self->chunk_size);
  return (void *)node;
}

void wt_pool_free(wt_pool_t *self, void *item)
{
  wt_pool_node_t *node;
  void *start = self->mem;
  void *end = &((byte_t *)self->mem)[self->size];
  if (item == NULL)
  {
    return;
  }

  if (!(start <= item && item < end))
  {
    return;
  }

  node = (wt_pool_node_t *)item;
  node->next = self->head;
  self->head = node;
}

void wt_pool_free_all(wt_pool_t *self)
{
  usize num_chunks = self->size / self->chunk_size;
  for (usize i = 0; i < num_chunks; ++i)
  {
    void *p = &((byte_t *)self->mem)[i * self->chunk_size];
    wt_pool_node_t *node = (wt_pool_node_t *)p;
    node->next = self->head;

    if (i > 0 && node->next == NULL)
    {
      __debugbreak();
    }

    self->head = node;
  }
}

static wt_buddy_block_t *buddy_block_next(wt_buddy_block_t *block)
{
  return (wt_buddy_block_t *)((u8 *)block + block->size);
}

static wt_buddy_block_t *buddy_block_split(wt_buddy_block_t *block, usize size)
{
  if (block != NULL && size != 0)
  {
    while (size < block->size)
    {
      usize sz = block->size >> 1;
      block->size = sz;
      block = buddy_block_next(block);
      block->size = sz;
      block->is_free = 1;
    }

    if (size <= block->size)
    {
      return block;
    }
  }
  return NULL;
}

static wt_buddy_block_t *buddy_block_find_best(wt_buddy_block_t *head, wt_buddy_block_t *tail, usize size)
{
  wt_buddy_block_t *best = NULL;
  wt_buddy_block_t *block = head;
  wt_buddy_block_t *buddy = buddy_block_next(block);

  if (buddy == tail && block->is_free)
  {
    return buddy_block_split(block, size);
  }

  while (block < tail && buddy < tail)
  {
    if (block->is_free && buddy->is_free && block->size == buddy->size)
    {
      block->size <<= 1;
      if (size <= block->size && (best == NULL || block->size <= best->size))
      {
        best = block;
      }

      block = buddy_block_next(buddy);
      if (block < tail)
      {
        buddy = buddy_block_next(block);
      }
      continue;
    }

    if (block->is_free && size <= block->size && (best == NULL || block->size <= best->size))
    {
      best = block;
    }

    if (buddy->is_free && size <= buddy->size && (best == NULL || buddy->size <= best->size))
    {
      best = buddy;
    }

    if (block->size <= buddy->size)
    {
      block = buddy_block_next(buddy);
      if (block < tail)
      {
        buddy = buddy_block_next(block);
      }
    }
    else
    {
      block = buddy;
      buddy = buddy_block_next(buddy);
    }
  }

  if (best != NULL)
  {
    return buddy_block_split(best, size);
  }

  return NULL;
}

static bool ispow2(usize x)
{
  return (x & (x - 1)) == 0;
}

wt_buddy_t wt_buddy_new(void *b, usize capacity)
{
  wt_buddy_t res = {0};
  WT_ASSERT(b != NULL);
  WT_ASSERT(ispow2(capacity) && "capacity must be power of 2");

  usize align = 16;
  if (align < sizeof(wt_buddy_block_t))
  {
    align = sizeof(wt_buddy_block_t);
  }
  WT_ASSERT((usize)b % align == 0 && "hunk is not aligned to minimum alignment");

  res.head = (wt_buddy_block_t *)b;
  res.head->size = capacity;
  res.head->is_free = 1;
  res.tail = buddy_block_next(res.head);
  res.align = align;
  return res;
}

static usize buddy_block_size_required(wt_buddy_t *b, usize size)
{
  usize actual_size = b->align;

  size += sizeof(wt_buddy_t);
  size = wt_align16(size);

  while (size > actual_size)
  {
    actual_size <<= 1;
  }

  return actual_size;
}

static void buddy_block_coalescence(wt_buddy_block_t *head, wt_buddy_block_t *tail)
{
  for (;;)
  {
    wt_buddy_block_t *block = head;
    wt_buddy_block_t *buddy = buddy_block_next(block);

    bool no_coalescence = true;
    while (block < tail && buddy < tail)
    {
      if (block->is_free && buddy->is_free && block->size == buddy->size)
      {
        block->size <<= 1;
        block = buddy_block_next(block);
        if (block < tail)
        {
          buddy = buddy_block_next(block);
          no_coalescence = false;
        }
      }
      else if (block->size < buddy->size)
      {
        block = buddy;
        buddy = buddy_block_next(buddy);
      }
      else
      {
        block = buddy_block_next(buddy);
        if (block < tail)
        {
          buddy = buddy_block_next(block);
        }
      }
    }

    if (no_coalescence)
    {
      break;
    }
  }
}

void *wt_buddy_alloc(wt_buddy_t *b, usize size)
{
  void *res = NULL;
  if (size != 0)
  {
    size_t actual_size = buddy_block_size_required(b, size);

    wt_buddy_block_t *found =
      buddy_block_find_best(b->head, b->tail, actual_size);
    if (found == NULL)
    {
      buddy_block_coalescence(b->head, b->tail);
      found = buddy_block_find_best(b->head, b->tail, actual_size);
    }

    if (found != NULL)
    {
      found->is_free = 0;
      res = (void *)((char *)found + b->align);
    }
  }
  memset(res, 0, size);
  return res;
}

void wt_buddy_release(wt_buddy_t *b, void *item)
{
  if (item != NULL)
  {
    wt_buddy_block_t *block;

    WT_ASSERT((void *)b->head <= item);
    WT_ASSERT(item < (void *)b->tail);

    block = (wt_buddy_block_t *)((char *)item - b->align);
    block->is_free = 1;
  }
}
