#ifndef WT_TYPES_H
#define WT_TYPES_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WT_I8_MIN  (i8)(0xff)
#define WT_I8_MAX  (i8)(0x7f)
#define WT_I16_MIN (i16)(0xffff)
#define WT_I16_MAX (i16)(0x7fff)
#define WT_I32_MIN (i32)(0xffffffff)
#define WT_I32_MAX (i32)(0x7fffffff)
#define WT_I64_MIN (i64)(0xffffffffffffffff)
#define WT_I64_MAX (i64)(0x7fffffffffffffff)

#define WT_U8_MAX  (u8)(0xff)
#define WT_U16_MAX (u16)(0xffff)
#define WT_U32_MAX (u32)(0xffffffff)
#define WT_U64_MAX (u64)(0xffffffffffffffff)

#define WT_PI_F32 3.141592653589793f
#define WT_PI_F64 3.141592653589793
#define WT_TAU_F32 (3.141592653589793f * 2.0f)
#define WT_TAU_F64 (3.141592653589793 * 2.0)

// scalar types
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;
typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;

typedef float     f32;
typedef double    f64;

typedef ptrdiff_t isize;
typedef size_t    usize;

typedef unsigned char byte_t;

// geometry types/functions
#define T i32
#define L i
#include <wt/geometry-types.inc.h>
#undef T
#undef L
#define T f32
#define L f
#include <wt/geometry-types.inc.h>
#undef T
#undef L
#define T f64
#define L d
#include <wt/geometry-types.inc.h>
#undef T
#undef L

typedef wt_vec2i_t wt_vec2_t;
typedef wt_vec3i_t wt_vec3_t;
typedef wt_vec4i_t wt_vec4_t;
typedef wt_recti_t wt_rect_t;

#define wt_vec2(x, y) wt_vec2i(x, y)
#define wt_vec3(x, y, z) wt_vec3i(x, y, z)
#define wt_rect(x, y, w, h) wt_recti(x, y, w, h)

wt_vec2f_t wt_vec2i_to_vec2f(wt_vec2i_t x);
wt_vec2d_t wt_vec2i_to_vec2d(wt_vec2i_t x);
wt_vec2i_t wt_vec2f_to_vec2i(wt_vec2f_t x);
wt_vec2d_t wt_vec2f_to_vec2d(wt_vec2f_t x);
wt_vec2f_t wt_vec2d_to_vec2f(wt_vec2d_t x);
wt_vec2i_t wt_vec2d_to_vec2i(wt_vec2d_t x);

wt_vec3f_t wt_vec3i_to_vec3f(wt_vec3i_t x);
wt_vec3d_t wt_vec3i_to_vec3d(wt_vec3i_t x);
wt_vec3i_t wt_vec3f_to_vec3i(wt_vec3f_t x);
wt_vec3d_t wt_vec3f_to_vec3d(wt_vec3f_t x);
wt_vec3f_t wt_vec3d_to_vec3f(wt_vec3d_t x);
wt_vec3i_t wt_vec3d_to_vec3i(wt_vec3d_t x);

wt_vec4f_t wt_vec4i_to_vec4f(wt_vec4i_t x);
wt_vec4d_t wt_vec4i_to_vec4d(wt_vec4i_t x);
wt_vec4i_t wt_vec4f_to_vec4i(wt_vec4f_t x);
wt_vec4d_t wt_vec4f_to_vec4d(wt_vec4f_t x);
wt_vec4f_t wt_vec4d_to_vec4f(wt_vec4d_t x);
wt_vec4i_t wt_vec4d_to_vec4i(wt_vec4d_t x);

wt_rectf_t wt_recti_to_rectf(wt_recti_t x);
wt_rectd_t wt_recti_to_rectd(wt_recti_t x);
wt_recti_t wt_rectf_to_recti(wt_rectf_t x);
wt_rectd_t wt_rectf_to_rectd(wt_rectf_t x);
wt_rectf_t wt_rectd_to_rectf(wt_rectd_t x);
wt_recti_t wt_rectd_to_recti(wt_rectd_t x);

f32        wt_vec3f_dot(wt_vec3f_t a, wt_vec3f_t b);
wt_vec3f_t wt_vec3f_cross(wt_vec3f_t a, wt_vec3f_t b);
f32        wt_vec3f_mag(wt_vec3f_t v);
wt_vec3f_t wt_vec3f_norm(wt_vec3f_t v);

typedef union
{
  f32 e[4][4];
  wt_vec4f_t rows[4];
} wt_mat4x4_t;

wt_mat4x4_t wt_mat4x4_identity(void);
wt_mat4x4_t wt_mat4x4_dup(wt_mat4x4_t m);
wt_vec4f_t  wt_mat4x4_row(wt_mat4x4_t m, int i);
wt_vec4f_t  wt_mat4x4_col(wt_mat4x4_t m, int i);
wt_mat4x4_t wt_mat4x4_scale(f32 x);
wt_mat4x4_t wt_mat4x4_scale_vec3(wt_vec3f_t v);
wt_mat4x4_t wt_mat4x4_mul(wt_mat4x4_t a, wt_mat4x4_t b);
wt_mat4x4_t wt_mat4x4_translate(wt_vec3f_t v);
wt_mat4x4_t wt_mat4x4_rotate(wt_vec3f_t v, f32 angle);
wt_mat4x4_t wt_mat4x4_ortho2d(f32 left, f32 right, f32 bottom, f32 top);
wt_mat4x4_t wt_mat4x4_ortho3d(f32 left, f32 right, f32 bottom, f32 top, f32 znear, f32 zfar);
wt_mat4x4_t wt_mat4x4_perspective(f32 fov, f32 aspect, f32 znear, f32 zfar);
wt_mat4x4_t wt_mat4x4_look_at(wt_vec3f_t eye, wt_vec3f_t center, wt_vec3f_t up);

// colors
#define WT_COLOR_BLACK       wt_color(0x1a, 0x1c, 0x2c, 0xff) // #1a1c2c
#define WT_COLOR_PURPLE      wt_color(0x5d, 0x27, 0x5d, 0xff) // #5d275d
#define WT_COLOR_RED         wt_color(0xb1, 0x3e, 0x53, 0xff) // #b13e53
#define WT_COLOR_ORANGE      wt_color(0xef, 0x7d, 0x57, 0xff) // #ef7d57
#define WT_COLOR_YELLOW      wt_color(0xff, 0xcd, 0x75, 0xff) // #ffcd75
#define WT_COLOR_LIGHT_GREEN wt_color(0xa7, 0xf0, 0x70, 0xff) // #a7f070
#define WT_COLOR_GREEN       wt_color(0x38, 0xb7, 0x64, 0xff) // #38b764
#define WT_COLOR_DARK_GREEN  wt_color(0x25, 0x71, 0x79, 0xff) // #257179
#define WT_COLOR_DARK_BLUE   wt_color(0x29, 0x36, 0x6f, 0xff) // #29366f
#define WT_COLOR_BLUE        wt_color(0x3b, 0x5d, 0xc9, 0xff) // #3b5dc9
#define WT_COLOR_LIGHT_BLUE  wt_color(0x41, 0xa6, 0xf6, 0xff) // #41a6f6
#define WT_COLOR_TEAL        wt_color(0x73, 0xef, 0xf7, 0xff) // #73eff7
#define WT_COLOR_WHITE       wt_color(0xf4, 0xf4, 0xf4, 0xff) // #f4f4f4
#define WT_COLOR_LIGHT_GRAY  wt_color(0x94, 0xb0, 0xc2, 0xff) // #94b0c2
#define WT_COLOR_GRAY        wt_color(0x56, 0x6c, 0x86, 0xff) // #566c86
#define WT_COLOR_DARK_GRAY   wt_color(0x33, 0x3c, 0x57, 0xff) // #333c57

typedef struct { u8  r, g, b, a; } wt_color_t;
typedef struct { f32 r, g, b, a; } wt_colorf_t;
typedef struct { f32 h, s, v, a; } wt_colorf_hsv_t;

wt_color_t       wt_color(u8 r, u8 g, u8 b, u8 a);
wt_color_t       wt_color_hex(const char *hex);
wt_colorf_t      wt_colorf(f32 r, f32 g, f32 b, f32 a);
wt_colorf_hsv_t  wt_colorf_hsv(f32 h, f32 s, f32 v, f32 a);

wt_colorf_t      wt_color_to_colorf(wt_color_t x);
wt_colorf_hsv_t  wt_color_to_colorf_hsv(wt_color_t x);
wt_color_t       wt_colorf_to_color(wt_colorf_t x);
wt_colorf_hsv_t  wt_colorf_to_colorf_hsv(wt_colorf_t x);
wt_color_t       wt_colorf_hsv_to_color(wt_colorf_hsv_t x);
wt_colorf_t      wt_colorf_hsv_to_colorf(wt_colorf_hsv_t x);

#endif
