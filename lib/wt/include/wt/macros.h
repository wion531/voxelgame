#ifndef WT_MACROS_H
#define WT_MACROS_H

#define WT_MIN(a, b) ((a) < (b) ? (a) : (b))
#define WT_MAX(a, b) ((a) > (b) ? (a) : (b))
#define WT_CLAMP(x, a, b) WT_MIN(WT_MAX((x), (a)), (b))

#define WT_KILOBYTES(x) ((usize)(x) << 10)
#define WT_MEGABYTES(x) ((usize)(x) << 20)
#define WT_GIGABYTES(x) ((usize)(x) << 30)
#define WT_TERABYTES(x) ((usize)(x) << 40)

#define WT_ARRAY_COUNT(x) (sizeof(x) / sizeof(*(x)))

#define WT_UNUSED(x) ((void)(x))

#if WT_DEBUG
#include <stdio.h>
#define WT_ASSERT(cond) do {\
    if (!(cond)) {\
      fprintf(stderr, "=== assertion failed! ===\nfile: %s\nline: %d\nexpr: %s\n",\
        __FILE__, __LINE__, #cond);\
      __debugbreak();\
    }\
  } while (0)
#else
#define WT_ASSERT(cond) (void)0
#endif

#endif
