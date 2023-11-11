#ifndef WT_CONTAINERS_H
#define WT_CONTAINERS_H

typedef struct
{
  u64 *keys;
  void *vals;
  usize capacity;
  usize capacity_log2;
  usize aligned_bucket_size, bucket_size;
} wt_hashmap_t;

wt_hashmap_t  wt_hashmap_new(void *buffer, usize size_bytes, usize bucket_size);
usize         wt_hashmap_buffer_size(usize bucket_size, usize num_buckets);
bool          wt_hashmap_insert(wt_hashmap_t *hm, u64 key, void *val);
void         *wt_hashmap_find(wt_hashmap_t *hm, u64 key);
void         *wt_hashmap_index(wt_hashmap_t *hm, u64 idx);
void          wt_hashmap_remove(wt_hashmap_t *hm, u64 key);
void          wt_hashmap_remove_index(wt_hashmap_t *hm, u64 idx);
void          wt_hashmap_clear(wt_hashmap_t *hm);

#endif
