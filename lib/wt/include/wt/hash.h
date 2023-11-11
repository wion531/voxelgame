#ifndef WT_HASH_H
#define WT_HASH_H

typedef struct
{
  u64 low;
  u64 high;
} wt_hash_u128_t;

u32            wt_hash_u32(void *data, usize size);
u64            wt_hash_u64(void *data, usize size);
wt_hash_u128_t wt_hash_u128(void *data, usize size);

#endif
