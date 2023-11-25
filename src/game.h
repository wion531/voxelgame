#ifndef GAME_H
#define GAME_H

#include <wt/wt.h>
#include "renderer.h"
#include "block.h"
#include "chunk.h"
#include "gpu.h"

#define INVENTORY_SIZE BLOCK_MAX

typedef struct
{
  void *mem;
  void *sys;
  void *gpu;
  void *ren;

  void *rng;
  void *job;

  void *block;
  void *chunk;
  void *world;

  void *player;
} game_modules_t;

typedef struct
{
  game_modules_t modules;

//  ren_chunk_t chunks[64 * 64];

  ren_texture_t debug_font;

  f64 delta_time;

/*
  wt_vec3f_t camera_pos;
  wt_vec3f_t camera_front;
  wt_vec2f_t camera_rotation;
*/
  gpu_buffer_t const_buffer;
  gpu_buffer_t vertex_buffer;
  gpu_shader_t shader;
  gpu_texture_t texture;

  block_id_t hotbar[INVENTORY_SIZE];
  isize hotbar_pos;

  block_id_t blocks[BLOCK_MAX];

  void *hunk;
} game_state_t;

void game_init(game_state_t *s);
bool game_tick(game_state_t *s);

void game_hot_unload(game_state_t *s);
void game_hot_reload(game_state_t *s);

game_state_t *game_get_state(void);

#endif
