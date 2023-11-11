#ifndef RNG_H
#define RNG_H

#include <wt/wt.h>

void rng_init(void);
void rng_mix(u64 x);
u64  rng_next(void);
i64  rng_next_range(i64 low, i64 high);
bool rng_next_bool(void);

#endif
