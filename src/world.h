#ifndef WORLD_H
#define WORLD_H

#include <wt/wt.h>
#include "block.h"

#define WORLD_MAX_CHUNKS_X 32
#define WORLD_MAX_CHUNKS_Z 32
#define WORLD_MAX_CHUNKS (WORLD_MAX_CHUNKS_X * WORLD_MAX_CHUNKS_Z)

typedef struct
{
  wt_vec3_t pos;
  wt_vec3_t neighbor;

  block_face_t face;

  bool hit;
} world_raycast_t;

void            world_init(void);
void            world_tick(void);
void            world_render(void);
void            world_generate(void);

void            world_save(void);
bool            world_load(void);

void            world_dbg_rebuild_meshes(void);

void            world_set_block(wt_vec3_t pos, block_id_t block);
block_id_t      world_get_block(wt_vec3_t pos);
bool            world_within_bounds(wt_vec3_t pos);

world_raycast_t world_raycast(int max_num_blocks);

#endif
