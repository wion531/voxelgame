#ifndef PLAYER_H
#define PLAYER_H

#include <wt/wt.h>

#define PLAYER_RADIUS 0.3f
#define PLAYER_HEIGHT 1.7f // height in blocks

void       player_init(void);
void       player_tick(void);

// if a block existed at that position, would the player intersect it?
bool       player_intersects_block_position(wt_vec3i_t pos);

wt_vec3f_t player_get_position(void);
wt_vec2f_t player_get_rotation(void);

wt_vec3f_t player_get_head_position(void);

#endif
