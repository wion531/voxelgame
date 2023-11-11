#include <wt/wt.h>

#if WT_COMPILER_MSVC

#define FORCE_INLINE	__forceinline

#include <stdlib.h>

#define ROTL32(x,y) _rotl(x,y)
#define ROTL64(x,y) _rotl64(x,y)

#define BIG_CONSTANT(x) (x)

#else	// defined(_MSC_VER)

#define FORCE_INLINE inline __attribute__((always_inline))

inline u32 rotl32(u32 x, i8 r )
{
  return (x << r) | (x >> (32 - r));
}

inline u64 rotl64(u64 x, i8 r )
{
  return (x << r) | (x >> (64 - r));
}

#define ROTL32(x,y) rotl32(x,y)
#define ROTL64(x,y) rotl64(x,y)

#define BIG_CONSTANT(x) (x##LLU)

#endif // !defined(_MSC_VER)

FORCE_INLINE u32 getblock32(const u32 *p, int i)
{
  return p[i];
}

FORCE_INLINE u64 getblock64(const u64 *p, int i)
{
  return p[i];
}

FORCE_INLINE u32 fmix32(u32 h)
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

FORCE_INLINE u64 fmix64(u64 k)
{
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;
}

u32 wt_hash_u32(void *data, usize size)
{
  const u32 seed = 0xb16b00b5;
  const byte_t *dp = (const byte_t*)data;
  const int nblocks = (int)(size / 4);

  u32 h1 = seed;

  const u32 c1 = 0xcc9e2d51;
  const u32 c2 = 0x1b873593;

  // === body ===
  const u32 *blocks = (const u32 *)(dp + nblocks*4);

  for(int i = -nblocks; i; i++)
  {
    uint32_t k1 = getblock32(blocks, i);

    k1 *= c1;
    k1 = ROTL32(k1,15);
    k1 *= c2;
    
    h1 ^= k1;
    h1 = ROTL32(h1,13); 
    h1 = h1*5+0xe6546b64;
  }

  //----------
  // tail

  const u8 *tail = (const u8*)(dp + nblocks*4);

  u32 k1 = 0;

  switch(size & 3)
  {
  case 3: k1 ^= tail[2] << 16;
  case 2: k1 ^= tail[1] << 8;
  case 1: k1 ^= tail[0];
          k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= size;
  h1 = fmix32(h1);
  return h1;
}

u64 wt_hash_u64(void *data, usize size)
{
  return wt_hash_u128(data, size).low;
}

wt_hash_u128_t wt_hash_u128(void *data, usize size)
{
  const u32 seed = 0xb15b00b5;
  const byte_t *dp = (const uint8_t*)data;
  const int nblocks = (int)(size / 16);

  u64 h1 = seed;
  u64 h2 = seed;

  const u64 c1 = BIG_CONSTANT(0x87c37b91114253d5);
  const u64 c2 = BIG_CONSTANT(0x4cf5ad432745937f);

  //----------
  // body

  const u64 *blocks = (const u64*)(dp);

  for(int i = 0; i < nblocks; i++)
  {
    u64 k1 = getblock64(blocks, i*2+0);
    u64 k2 = getblock64(blocks, i*2+1);

    k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;

    h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

    k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

    h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
  }

  //----------
  // tail

  const u8 *tail = (const u8*)(dp + nblocks*16);

  u64 k1 = 0;
  u64 k2 = 0;

  switch(size & 15)
  {
  case 15: k2 ^= ((u64)tail[14]) << 48;
  case 14: k2 ^= ((u64)tail[13]) << 40;
  case 13: k2 ^= ((u64)tail[12]) << 32;
  case 12: k2 ^= ((u64)tail[11]) << 24;
  case 11: k2 ^= ((u64)tail[10]) << 16;
  case 10: k2 ^= ((u64)tail[ 9]) << 8;
  case  9: k2 ^= ((u64)tail[ 8]) << 0;
           k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;

  case  8: k1 ^= ((u64)tail[ 7]) << 56;
  case  7: k1 ^= ((u64)tail[ 6]) << 48;
  case  6: k1 ^= ((u64)tail[ 5]) << 40;
  case  5: k1 ^= ((u64)tail[ 4]) << 32;
  case  4: k1 ^= ((u64)tail[ 3]) << 24;
  case  3: k1 ^= ((u64)tail[ 2]) << 16;
  case  2: k1 ^= ((u64)tail[ 1]) << 8;
  case  1: k1 ^= ((u64)tail[ 0]) << 0;
           k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= size; h2 ^= size;

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;
  h2 += h1;

  return (wt_hash_u128_t){ h1, h2 };
}
