#ifndef CHUNK_H
#define CHUNK_H

#define CHUNK_MAX 4096

#include <wt/wt.h>
#include "constants.h"
#include "block.h"
#include "renderer.h"

typedef struct
{
  wt_vec2_t position;
  block_id_t blocks[CHUNK_NUM_BLOCKS];

  ren_chunk_t mesh;
  bool dirty;
} chunk_t;

void       chunk_init(void);
chunk_t   *chunk_new(wt_vec2_t pos);
void       chunk_gen(chunk_t *c);
void       chunk_set_block(chunk_t *c, wt_vec3_t position, block_id_t block);
block_id_t chunk_get_block(chunk_t *c, wt_vec3_t position);
void       chunk_rebuild_mesh(chunk_t *c);
void       chunk_render(chunk_t *c);
void       chunk_free(chunk_t *c);

#endif
