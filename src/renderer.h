#ifndef RENDERER_H
#define RENDERER_H

#include <wt/wt.h>
#include "block.h"

typedef struct
{
  int width, height;
  void *data;
} ren_texture_t;

typedef struct ren_chunk_t *ren_chunk_t;

void          ren_init(void);

// todo: get rid of window size parameter, as it is now useless
void          ren_frame_begin(wt_vec2_t window_size);
void          ren_frame_end(void);

ren_texture_t ren_texture_new(wt_color_t *pixels, int width, int height);
ren_texture_t ren_texture_load_from_file(const char *filename);
void          ren_texture_free(ren_texture_t tx);

ren_chunk_t   ren_chunk_new(wt_vec2_t position);
void          ren_chunk_generate_mesh(ren_chunk_t c, block_id_t *blocks);
void          ren_chunk_free(ren_chunk_t c);

void          ren_camera_set(wt_mat4x4_t mtx);

void          ren_draw_box(wt_rect_t rect, wt_color_t color);
void          ren_draw_rect(wt_rect_t rect, wt_color_t color);
void          ren_draw_sprite(ren_texture_t tx, wt_vec2_t pos);
void          ren_draw_sprite_scaled(ren_texture_t tx, wt_vec2_t pos, i32 scale);
void          ren_draw_sprite_stretch(ren_texture_t tx, wt_rect_t src, wt_rect_t dst);
void          ren_draw_sprite_ex(ren_texture_t tx, wt_rect_t src, wt_rect_t dst,
                wt_vec2_t origin, float rotation, wt_color_t tint);
void          ren_draw_chunk(ren_chunk_t c);

#endif
