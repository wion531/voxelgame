#ifndef BLOCK_H
#define BLOCK_H

#include <wt/wt.h>

#define BLOCK_MAX_COUNT 256
#define BLOCK_NAME_LEN 32

typedef u32 block_id_t;

// todo: either generate this or get rid of it
typedef enum
{
  BLOCK_AIR,
  BLOCK_DIRT,
  BLOCK_GRASS_BLOCK,
  BLOCK_PLANKS,
  BLOCK_COBBLESTONE,
  BLOCK_TILE,
  BLOCK_LOG,
  BLOCK_LEAVES,

  BLOCK_MAX,
} block_type_t;

typedef enum
{
  BLOCK_FACE_TOP,
  BLOCK_FACE_BOTTOM,
  BLOCK_FACE_FRONT,
  BLOCK_FACE_BACK,
  BLOCK_FACE_LEFT,
  BLOCK_FACE_RIGHT,
} block_face_t;

typedef struct
{
  char name[BLOCK_NAME_LEN];
  wt_vec2_t atlas_tiles[6];

  bool solid;
} block_info_t;

void          block_mgr_init(void);

block_id_t    block_new(block_info_t *info);
block_info_t *block_get_info(block_id_t id);

#endif
