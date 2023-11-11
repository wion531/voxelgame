// This is a stupid hack for emulating templates in C. We define a few macros to specify the type
// and use those macros to generate code.
// I might eventually decide against this and just write out the functions, but this could be useful
// if it does end up working out in the long run.
// I've got to say - this is simultaneously the most beautiful and the most ugly thing I've ever made.

// This file deliberately has no #include guard. Don't write one.
// However, do #define GEOMETRY_TYPES_IMPL for types.c.

#ifndef T
  #error T must be defined for template files.
#endif

#ifndef L
  #error L must be defined for template files (single letter suffix for type names).
#endif

#define GLUE_(x, y) x##y
#define GLUE(x, y) GLUE_(x, y)

#define VEC2 GLUE(wt_vec2, L)
#define VEC3 GLUE(wt_vec3, L)
#define VEC4 GLUE(wt_vec4, L)
#define RECT GLUE(wt_rect, L)

#define VEC2_NAME(name) GLUE(VEC2, GLUE(_, name))
#define VEC3_NAME(name) GLUE(VEC3, GLUE(_, name))
#define VEC4_NAME(name) GLUE(VEC4, GLUE(_, name))
#define RECT_NAME(name) GLUE(RECT, GLUE(_, name))

#define VEC2_T VEC2_NAME(t)
#define VEC3_T VEC3_NAME(t)
#define VEC4_T VEC4_NAME(t)
#define RECT_T RECT_NAME(t)

#ifndef GEOMETRY_TYPES_IMPL // we don't want to redefine types unnecessarily
typedef struct { T x, y; } VEC2_T;
typedef struct { T x, y, z; } VEC3_T;
typedef struct { T x, y, z, w; } VEC4_T;
typedef struct { T x, y, w, h; } RECT_T;
#endif

VEC2_T VEC2(T x, T y);
VEC3_T VEC3(T x, T y, T z);
VEC4_T VEC4(T x, T y, T z, T w);
RECT_T RECT(T x, T y, T w, T h);

// add/sub/mul/div with a constant
VEC2_T GLUE(VEC2_NAME(add), GLUE(_, T))(VEC2_T a, T b);
VEC2_T GLUE(VEC2_NAME(sub), GLUE(_, T))(VEC2_T a, T b);
VEC2_T GLUE(VEC2_NAME(mul), GLUE(_, T))(VEC2_T a, T b);
VEC2_T GLUE(VEC2_NAME(div), GLUE(_, T))(VEC2_T a, T b);

VEC3_T GLUE(VEC3_NAME(add), GLUE(_, T))(VEC3_T a, T b);
VEC3_T GLUE(VEC3_NAME(sub), GLUE(_, T))(VEC3_T a, T b);
VEC3_T GLUE(VEC3_NAME(mul), GLUE(_, T))(VEC3_T a, T b);
VEC3_T GLUE(VEC3_NAME(div), GLUE(_, T))(VEC3_T a, T b);

VEC4_T GLUE(VEC4_NAME(add), GLUE(_, T))(VEC4_T a, T b);
VEC4_T GLUE(VEC4_NAME(sub), GLUE(_, T))(VEC4_T a, T b);
VEC4_T GLUE(VEC4_NAME(mul), GLUE(_, T))(VEC4_T a, T b);
VEC4_T GLUE(VEC4_NAME(div), GLUE(_, T))(VEC4_T a, T b);

// add/sub/mul/div with another vector/rect
VEC2_T VEC2_NAME(add)(VEC2_T a, VEC2_T b);
VEC2_T VEC2_NAME(sub)(VEC2_T a, VEC2_T b);
VEC2_T VEC2_NAME(mul)(VEC2_T a, VEC2_T b);
VEC2_T VEC2_NAME(div)(VEC2_T a, VEC2_T b);

VEC3_T VEC3_NAME(add)(VEC3_T a, VEC3_T b);
VEC3_T VEC3_NAME(sub)(VEC3_T a, VEC3_T b);
VEC3_T VEC3_NAME(mul)(VEC3_T a, VEC3_T b);
VEC3_T VEC3_NAME(div)(VEC3_T a, VEC3_T b);

VEC4_T VEC4_NAME(add)(VEC4_T a, VEC4_T b);
VEC4_T VEC4_NAME(sub)(VEC4_T a, VEC4_T b);
VEC4_T VEC4_NAME(mul)(VEC4_T a, VEC4_T b);
VEC4_T VEC4_NAME(div)(VEC4_T a, VEC4_T b);

RECT_T RECT_NAME(add)(RECT_T a, RECT_T b);
RECT_T RECT_NAME(sub)(RECT_T a, RECT_T b);
RECT_T RECT_NAME(mul)(RECT_T a, RECT_T b);
RECT_T RECT_NAME(div)(RECT_T a, RECT_T b);

// rect utilities
VEC2_T RECT_NAME(pos)(RECT_T x);
VEC2_T RECT_NAME(size)(RECT_T x);
VEC2_T RECT_NAME(top_left)(RECT_T x);
VEC2_T RECT_NAME(top_right)(RECT_T x);
VEC2_T RECT_NAME(bottom_left)(RECT_T x);
VEC2_T RECT_NAME(bottom_right)(RECT_T x);

RECT_T RECT_NAME(expand)(RECT_T x, T n);

#ifdef GEOMETRY_TYPES_IMPL

VEC2_T VEC2(T x, T y) { return (VEC2_T){ x, y }; }
VEC3_T VEC3(T x, T y, T z) { return (VEC3_T){ x, y, z }; }
VEC4_T VEC4(T x, T y, T z, T w) { return (VEC4_T){ x, y, z, w }; }
RECT_T RECT(T x, T y, T w, T h) { return (RECT_T){ x, y, w, h }; }

VEC2_T GLUE(VEC2_NAME(add), GLUE(_, T))(VEC2_T a, T b) { return VEC2(a.x + b, a.y + b); }
VEC2_T GLUE(VEC2_NAME(sub), GLUE(_, T))(VEC2_T a, T b) { return VEC2(a.x - b, a.y - b); }
VEC2_T GLUE(VEC2_NAME(mul), GLUE(_, T))(VEC2_T a, T b) { return VEC2(a.x * b, a.y * b); }
VEC2_T GLUE(VEC2_NAME(div), GLUE(_, T))(VEC2_T a, T b) { return VEC2(a.x / b, a.y / b); }

VEC3_T GLUE(VEC3_NAME(add), GLUE(_, T))(VEC3_T a, T b) { return VEC3(a.x + b, a.y + b, a.z + b); }
VEC3_T GLUE(VEC3_NAME(sub), GLUE(_, T))(VEC3_T a, T b) { return VEC3(a.x - b, a.y - b, a.z - b); }
VEC3_T GLUE(VEC3_NAME(mul), GLUE(_, T))(VEC3_T a, T b) { return VEC3(a.x * b, a.y * b, a.z * b); }
VEC3_T GLUE(VEC3_NAME(div), GLUE(_, T))(VEC3_T a, T b) { return VEC3(a.x / b, a.y / b, a.z / b); }

VEC4_T GLUE(VEC4_NAME(add), GLUE(_, T))(VEC4_T a, T b) { return VEC4(a.x + b, a.y + b, a.z + b, a.w + b); }
VEC4_T GLUE(VEC4_NAME(sub), GLUE(_, T))(VEC4_T a, T b) { return VEC4(a.x - b, a.y - b, a.z - b, a.w - b); }
VEC4_T GLUE(VEC4_NAME(mul), GLUE(_, T))(VEC4_T a, T b) { return VEC4(a.x * b, a.y * b, a.z * b, a.w * b); }
VEC4_T GLUE(VEC4_NAME(div), GLUE(_, T))(VEC4_T a, T b) { return VEC4(a.x / b, a.y / b, a.z / b, a.w / b); }

VEC2_T VEC2_NAME(add)(VEC2_T a, VEC2_T b) { return VEC2(a.x + b.x, a.y + b.y); }
VEC2_T VEC2_NAME(sub)(VEC2_T a, VEC2_T b) { return VEC2(a.x - b.x, a.y - b.y); }
VEC2_T VEC2_NAME(mul)(VEC2_T a, VEC2_T b) { return VEC2(a.x * b.x, a.y * b.y); }
VEC2_T VEC2_NAME(div)(VEC2_T a, VEC2_T b) { return VEC2(a.x / b.x, a.y / b.y); }

VEC3_T VEC3_NAME(add)(VEC3_T a, VEC3_T b) { return VEC3(a.x + b.x, a.y + b.y, a.z + b.z); }
VEC3_T VEC3_NAME(sub)(VEC3_T a, VEC3_T b) { return VEC3(a.x - b.x, a.y - b.y, a.z - b.z); }
VEC3_T VEC3_NAME(mul)(VEC3_T a, VEC3_T b) { return VEC3(a.x * b.x, a.y * b.y, a.z * b.z); }
VEC3_T VEC3_NAME(div)(VEC3_T a, VEC3_T b) { return VEC3(a.x / b.x, a.y / b.y, a.z / b.z); }

VEC4_T VEC4_NAME(add)(VEC4_T a, VEC4_T b) { return VEC4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
VEC4_T VEC4_NAME(sub)(VEC4_T a, VEC4_T b) { return VEC4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w); }
VEC4_T VEC4_NAME(mul)(VEC4_T a, VEC4_T b) { return VEC4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }
VEC4_T VEC4_NAME(div)(VEC4_T a, VEC4_T b) { return VEC4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }

RECT_T RECT_NAME(add)(RECT_T a, RECT_T b) { return RECT(a.x + b.x, a.y + b.y, a.w + b.w, a.h + b.h); }
RECT_T RECT_NAME(sub)(RECT_T a, RECT_T b) { return RECT(a.x - b.x, a.y - b.y, a.w - b.w, a.h - b.h); }
RECT_T RECT_NAME(mul)(RECT_T a, RECT_T b) { return RECT(a.x * b.x, a.y * b.y, a.w * b.w, a.h * b.h); }
RECT_T RECT_NAME(div)(RECT_T a, RECT_T b) { return RECT(a.x / b.x, a.y / b.y, a.w / b.w, a.h / b.h); }

VEC2_T RECT_NAME(pos)(RECT_T x)          { return VEC2(x.x, x.y); }
VEC2_T RECT_NAME(size)(RECT_T x)         { return VEC2(x.w, x.h); }
VEC2_T RECT_NAME(top_left)(RECT_T x)     { return VEC2(x.x,           x.y          ); }
VEC2_T RECT_NAME(top_right)(RECT_T x)    { return VEC2(x.x + x.w - 1, x.y          ); }
VEC2_T RECT_NAME(bottom_left)(RECT_T x)  { return VEC2(x.x,           x.y + x.h - 1); }
VEC2_T RECT_NAME(bottom_right)(RECT_T x) { return VEC2(x.x + x.w - 1, x.y + x.h - 1); }

RECT_T RECT_NAME(expand)(RECT_T x, T n)  { return RECT(x.x - n, x.y - n, x.w + n + n, x.h + n + n); }

#endif // GEOMETRY_TYPES_IMPL

// cleanup
#undef GLUE_
#undef GLUE
#undef VEC2
#undef VEC3
#undef VEC4
#undef RECT
#undef VEC2_NAME
#undef VEC3_NAME
#undef VEC4_NAME
#undef RECT_NAME
#undef VEC2_T
#undef VEC3_T
#undef VEC4_T
#undef RECT_T
