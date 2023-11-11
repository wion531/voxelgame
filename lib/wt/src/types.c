#include <wt/wt.h>
#include <math.h>

#define GEOMETRY_TYPES_IMPL

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

wt_vec2f_t wt_vec2i_to_vec2f(wt_vec2i_t x) { return wt_vec2f(x.x, x.y); }
wt_vec2d_t wt_vec2i_to_vec2d(wt_vec2i_t x) { return wt_vec2d(x.x, x.y); }
wt_vec2i_t wt_vec2f_to_vec2i(wt_vec2f_t x) { return wt_vec2i(x.x, x.y); }
wt_vec2d_t wt_vec2f_to_vec2d(wt_vec2f_t x) { return wt_vec2d(x.x, x.y); }
wt_vec2f_t wt_vec2d_to_vec2f(wt_vec2d_t x) { return wt_vec2f(x.x, x.y); }
wt_vec2i_t wt_vec2d_to_vec2i(wt_vec2d_t x) { return wt_vec2i(x.x, x.y); }

wt_vec3f_t wt_vec3i_to_vec3f(wt_vec3i_t x) { return wt_vec3f(x.x, x.y, x.z); }
wt_vec3d_t wt_vec3i_to_vec3d(wt_vec3i_t x) { return wt_vec3d(x.x, x.y, x.z); }
wt_vec3i_t wt_vec3f_to_vec3i(wt_vec3f_t x) { return wt_vec3i(x.x, x.y, x.z); }
wt_vec3d_t wt_vec3f_to_vec3d(wt_vec3f_t x) { return wt_vec3d(x.x, x.y, x.z); }
wt_vec3f_t wt_vec3d_to_vec3f(wt_vec3d_t x) { return wt_vec3f(x.x, x.y, x.z); }
wt_vec3i_t wt_vec3d_to_vec3i(wt_vec3d_t x) { return wt_vec3i(x.x, x.y, x.z); }

wt_rectf_t wt_recti_to_rectf(wt_recti_t x) { return wt_rectf(x.x, x.y, x.w, x.h); }
wt_rectd_t wt_recti_to_rectd(wt_recti_t x) { return wt_rectd(x.x, x.y, x.w, x.h); }
wt_recti_t wt_rectf_to_recti(wt_rectf_t x) { return wt_recti(x.x, x.y, x.w, x.h); }
wt_rectd_t wt_rectf_to_rectd(wt_rectf_t x) { return wt_rectd(x.x, x.y, x.w, x.h); }
wt_rectf_t wt_rectd_to_rectf(wt_rectd_t x) { return wt_rectf(x.x, x.y, x.w, x.h); }
wt_recti_t wt_rectd_to_recti(wt_rectd_t x) { return wt_recti(x.x, x.y, x.w, x.h); }

f32 wt_vec3f_dot(wt_vec3f_t a, wt_vec3f_t b)
{
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

wt_vec3f_t wt_vec3f_cross(wt_vec3f_t a, wt_vec3f_t b)
{
  return wt_vec3f(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x);
}

f32 wt_vec3f_mag(wt_vec3f_t v)
{
  return sqrtf(wt_vec3f_dot(v, v));
}

wt_vec3f_t wt_vec3f_norm(wt_vec3f_t v)
{
  f32 mag = wt_vec3f_mag(v);
  return wt_vec3f_div_f32(v, mag);
}

//////////////////////////////////////////////////////////////////////////////
// NOTE: index mat4x4 with [col][row]. we're using row-major representation //
//////////////////////////////////////////////////////////////////////////////

wt_mat4x4_t wt_mat4x4_identity(void)
{
  return (wt_mat4x4_t){
    .rows[0] = { 1.0f, 0.0f, 0.0f, 0.0f },
    .rows[1] = { 0.0f, 1.0f, 0.0f, 0.0f },
    .rows[2] = { 0.0f, 0.0f, 1.0f, 0.0f },
    .rows[3] = { 0.0f, 0.0f, 0.0f, 1.0f },
  };
}


wt_mat4x4_t wt_mat4x4_scale(f32 x)
{
  wt_mat4x4_t res = wt_mat4x4_identity();
  res.e[0][0] = x;
  res.e[1][1] = x;
  res.e[2][2] = x;
  return res;
}

wt_mat4x4_t wt_mat4x4_scale_vec3(wt_vec3f_t v)
{
  wt_mat4x4_t res = wt_mat4x4_identity();
  res.e[0][0] = v.x;
  res.e[1][1] = v.y;
  res.e[2][2] = v.z;
  return res;
}

wt_mat4x4_t wt_mat4x4_mul(wt_mat4x4_t a, wt_mat4x4_t b)
{
  wt_mat4x4_t res = { 0 };
  for (u32 j = 0; j < 4; ++j)
  {
    for (u32 i = 0; i < 4; ++i)
    {
      res.e[i][j] =
        a.e[i][0] * b.e[0][j] +
        a.e[i][1] * b.e[1][j] +
        a.e[i][2] * b.e[2][j] +
        a.e[i][3] * b.e[3][j];
    }
  }
  return res;
}

wt_mat4x4_t wt_mat4x4_translate(wt_vec3f_t v)
{
  wt_mat4x4_t res = wt_mat4x4_identity();
  res.e[0][3] = v.x;
  res.e[1][3] = v.y;
  res.e[2][3] = v.z;
  res.e[3][3] = 1.0f;
  return res;
}

wt_mat4x4_t wt_mat4x4_rotate(wt_vec3f_t v, f32 angle)
{
  f32 rad = angle * WT_TAU_F32;
  f32 c = cosf(rad), s = sinf(rad);

  wt_vec3f_t axis = wt_vec3f_norm(v);
  wt_vec3f_t t = wt_vec3f_mul_f32(axis, 1.0f - c);

  wt_mat4x4_t res = { 0 };
  res.e[0][0] = c + t.x*axis.x;
  res.e[1][0] = 0 + t.x*axis.y + s*axis.z;
  res.e[2][0] = 0 + t.x*axis.z - s*axis.y;
  res.e[3][0] = 0;

  res.e[0][1] = 0 + t.y*axis.x - s*axis.z;
  res.e[1][1] = c + t.y*axis.y;
  res.e[2][1] = 0 + t.y*axis.z + s*axis.x;
  res.e[3][1] = 0;

  res.e[0][2] = 0 + t.z*axis.x + s*axis.y;
  res.e[1][2] = 0 + t.z*axis.y - s*axis.x;
  res.e[2][2] = c + t.z*axis.z;
  res.e[3][2] = 0;

  return res;
}

wt_mat4x4_t wt_mat4x4_ortho2d(f32 left, f32 right, f32 bottom, f32 top)
{
  wt_mat4x4_t res = wt_mat4x4_identity();
  res.e[0][0] = 2.0f / (right - left);
  res.e[1][1] = 2.0f / (top - bottom);
  res.e[2][2] = -1.0f;
  res.e[0][3] = -(right + left) / (right - left);
  res.e[1][3] = -(top + bottom) / (top - bottom);
  return res;
}

wt_mat4x4_t wt_mat4x4_ortho3d(f32 left, f32 right, f32 bottom, f32 top, f32 znear, f32 zfar)
{
  wt_mat4x4_t res = wt_mat4x4_identity();
  res.e[0][0] = +2.0f / (right - left);
  res.e[1][1] = +2.0f / (top - bottom);
  res.e[2][2] = -2.0f / (zfar - znear);
  res.e[0][3] = -(right + left) / (right - left);
  res.e[1][3] = -(top + bottom) / (top - bottom);
  res.e[2][3] = -(zfar + znear) / (zfar - znear);
  return res;
}

wt_mat4x4_t wt_mat4x4_perspective(f32 fov, f32 aspect, f32 znear, f32 zfar)
{
  f32 tan_half_fov = tanf(0.5f * fov * WT_TAU_F32);
  wt_mat4x4_t res = { 0 };

  res.e[0][0] = 1.0f / (aspect * tan_half_fov);
  res.e[1][1] = 1.0f / (tan_half_fov);
  res.e[2][2] = -(zfar + znear) / (zfar - znear);
  
  res.e[3][2] = -1.0f;
  res.e[2][3] = -2.0f*zfar*znear / (zfar - znear);

  return res;
}

wt_mat4x4_t wt_mat4x4_look_at(wt_vec3f_t eye, wt_vec3f_t center, wt_vec3f_t up)
{
  wt_mat4x4_t res = wt_mat4x4_identity();

  wt_vec3f_t f = wt_vec3f_norm(wt_vec3f_sub(center, eye));
  wt_vec3f_t s = wt_vec3f_norm(wt_vec3f_cross(f, up));
  wt_vec3f_t u = wt_vec3f_norm(wt_vec3f_cross(s, f));

  res.e[0][0] = +s.x;
  res.e[0][1] = +s.y;
  res.e[0][2] = +s.z;
  res.e[1][0] = +u.x;
  res.e[1][1] = +u.y;
  res.e[1][2] = +u.z;
  res.e[2][0] = -f.x;
  res.e[2][1] = -f.y;
  res.e[2][2] = -f.z;
  res.e[0][3] = -wt_vec3f_dot(s, eye);
  res.e[1][3] = -wt_vec3f_dot(u, eye);
  res.e[2][3] = wt_vec3f_dot(f, eye);

  return res;
}

wt_color_t wt_color(u8 r, u8 g, u8 b, u8 a) { return (wt_color_t){ r, g, b, a }; }
wt_color_t wt_color_hex(const char *hex)
{
  // TODO
  WT_UNUSED(hex);
  return (wt_color_t){ 0 };
}

wt_colorf_t wt_colorf(f32 r, f32 g, f32 b, f32 a) { return (wt_colorf_t){ r, g, b, a }; }
wt_colorf_hsv_t wt_colorf_hsv(f32 h, f32 s, f32 v, f32 a) { return (wt_colorf_hsv_t){ h, s, v, a }; }

wt_colorf_t wt_color_to_colorf(wt_color_t x)
{
  return wt_colorf((f32)x.r / 255.0f, (f32)x.g / 255.0f, (f32)x.b / 255.0f, (f32)x.a / 255.0f);
}

wt_colorf_hsv_t wt_color_to_colorf_hsv(wt_color_t x)
{
  return wt_colorf_to_colorf_hsv(wt_color_to_colorf(x));
}

wt_color_t wt_colorf_to_color(wt_colorf_t x)
{
  return wt_color((u8)(x.r * 255.0f), (u8)(x.g * 255.0f), (u8)(x.b * 255.0f), (u8)(x.a * 255.0f));
}

wt_colorf_hsv_t wt_colorf_to_colorf_hsv(wt_colorf_t rgb)
{
  f32 cmax = WT_MAX(WT_MAX(rgb.r, rgb.g), rgb.b);
  f32 cmin = WT_MIN(WT_MIN(rgb.r, rgb.g), rgb.b);
  f32 delta = cmax - cmin;
  bool cmax_is_r = (rgb.r > rgb.g && rgb.r > rgb.b);
  bool cmax_is_g = (rgb.g > rgb.r && rgb.g > rgb.b);
  bool cmax_is_b = (rgb.b > rgb.r && rgb.b > rgb.g);
  f32 h = (cmax_is_r ? (rgb.g - rgb.b) / delta + 0
         : cmax_is_g ? (rgb.b - rgb.r) / delta + 2
         : cmax_is_b ? (rgb.r - rgb.g) / delta + 4
         : 0);
  f32 s = cmax == 0 ? 0 : (delta / cmax);
  f32 v = cmax;
  return wt_colorf_hsv(h / 6.0f, s, v, rgb.a);
}

wt_color_t wt_colorf_hsv_to_color(wt_colorf_hsv_t x)
{
  return wt_colorf_to_color(wt_colorf_hsv_to_colorf(x));
}

wt_colorf_t wt_colorf_hsv_to_colorf(wt_colorf_hsv_t hsv)
{
  f32 h = fmodf(hsv.h * 360.0f, 360.0f);
  f32 s = hsv.s;
  f32 v = hsv.v;

  f32 c = v * s;
  f32 x = c * (1 - fabs(fmodf((h / 60.0f), 2) - 1));
  f32 m = v - c;

  f32 r, g, b;
  if ((h >= 0.0f && h < 60.0f) || (h >= 360.0f && h < 420.0f)) { r = c; g = x; b = 0; }
  else if (h >= 60.0f && h < 120.0f)                           { r = x; g = c; b = 0; }
  else if (h >= 120.0f && h < 180.0f)                          { r = 0; g = c; b = x; }
  else if (h >= 180.0f && h < 240.0f)                          { r = 0; g = x; b = c; }
  else if (h >= 240.0f && h < 300.0f)                          { r = x; g = 0; b = c; }
  else                                                         { r = c; g = 0; b = x; }

  return wt_colorf(r + m, g + m, b + m, hsv.a);
}
