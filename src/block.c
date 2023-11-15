#include "block.h"
#include "game.h"
#include "memory.h"

typedef struct
{
//  wt_hashmap_t block_table;
  block_info_t block_table[BLOCK_MAX_COUNT];
  usize num_blocks;
} block_state_t;

static block_state_t *get_state(void)
{
  return game_get_state()->modules.block;
}

void block_mgr_init(void)
{
  game_state_t *gs = game_get_state();
  block_state_t *s = gs->modules.block = mem_hunk_push(sizeof(block_state_t));
  
//  usize buffer_size = wt_hashmap_buffer_size(sizeof(block_info_t), BLOCK_MAX_COUNT);
//  s->block_table = wt_hashmap_new(mem_hunk_push(buffer_size), buffer_size, sizeof(block_info_t));
}

block_id_t block_new(block_info_t *info)
{
  block_state_t *s = get_state();
//  u64 key = wt_hash_u32(info->name, BLOCK_NAME_LEN);
//  wt_hashmap_insert(&s->block_table, key, info);
  s->block_table[s->num_blocks++] = *info;
  return s->num_blocks - 1;
}

block_info_t *block_get_info(block_id_t id)
{
  block_state_t *s = get_state();
//  return wt_hashmap_find(&s->block_table, id);
//  if (id < s->num_blocks)
  return &s->block_table[id];
}
