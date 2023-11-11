#include <wt/wt.h>
#include <string.h>

static usize align_bucket_size(usize x)
{
  if (x > 8)
  {
    x = (x + 0xf) & ~0xf;
  }
  return x;
}

wt_hashmap_t wt_hashmap_new(void *buffer, usize size_bytes, usize bucket_size)
{
  usize aligned_bucket_size = align_bucket_size(bucket_size);

  WT_ASSERT(buffer != NULL);
  WT_ASSERT(size_bytes > aligned_bucket_size + sizeof(u64));

  usize num_buckets = size_bytes / (aligned_bucket_size + sizeof(u64));
  WT_ASSERT((num_buckets & (num_buckets - 1)) == 0 && "number of buckets must be power of 2");

  wt_hashmap_t res = { 0 };
  res.keys = buffer;
  res.vals = (byte_t*)buffer + (num_buckets * sizeof(u64));
  res.capacity = num_buckets;
  res.bucket_size = bucket_size;
  res.aligned_bucket_size = aligned_bucket_size;
  return res;
}

usize wt_hashmap_buffer_size(usize bucket_size, usize num_buckets)
{
  bucket_size = align_bucket_size(bucket_size);
  return num_buckets * (bucket_size + sizeof(u64));
}

typedef struct
{
  u64 index;
  void *item;
} probe_result_t;

static probe_result_t probe(wt_hashmap_t *hm, u64 key, bool find_existing)
{
  u64 needle = find_existing ? key : 0;
  u64 search_index = key & (hm->capacity - 1); //% hm->capacity;
  for (u64 i = 0; i < hm->capacity; ++i)
  {
    if (hm->keys[search_index] == needle)
    {
      hm->keys[search_index] = key;
      
      void *item = wt_hashmap_index(hm, search_index);
      probe_result_t res = { search_index, item };
      return res;
    }
    search_index = (search_index + 1) & (hm->capacity - 1);
  }
  return (probe_result_t){ 0 };
}

bool wt_hashmap_insert(wt_hashmap_t *hm, u64 key, void *val)
{
  probe_result_t slot = probe(hm, key, true);
  if (slot.item == NULL)
  {
    slot = probe(hm, key, false);
    if (slot.item == NULL)
      return false;
  }

  memcpy(slot.item, val, hm->bucket_size);
  return true;
}

void *wt_hashmap_find(wt_hashmap_t *hm, u64 key)
{
  probe_result_t slot = probe(hm, key, true);
  if (slot.item == NULL)
  {
    return NULL;
  }
  return slot.item;
}

void *wt_hashmap_index(wt_hashmap_t *hm, u64 idx)
{
  if (hm->keys[idx] != 0)
    return (byte_t*)hm->vals + idx * hm->aligned_bucket_size;
  else
    return NULL;
}

void wt_hashmap_remove(wt_hashmap_t *hm, u64 key)
{
  probe_result_t slot = probe(hm, key, true);
  if (slot.item != NULL)
  {
    hm->keys[slot.index] = 0;
    memset(wt_hashmap_index(hm, slot.index), 0, hm->aligned_bucket_size);
  }
}

void wt_hashmap_remove_index(wt_hashmap_t *hm, u64 idx)
{
  WT_ASSERT(wt_hashmap_index(hm, idx) != NULL);

  hm->keys[idx] = 0;
  memset(wt_hashmap_index(hm, idx), 0, hm->aligned_bucket_size);
}

void wt_hashmap_clear(wt_hashmap_t *hm)
{
  memset(hm->keys, 0, sizeof(u64) * hm->capacity);
  memset(hm->vals, 0, hm->aligned_bucket_size * hm->capacity);
}
