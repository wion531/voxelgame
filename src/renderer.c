#include "renderer.h"
#include "game.h"
#include "memory.h"
#include "gpu.h"

#define MAX_SOLID_VERTICES 1000
#define MAX_SOLID_INDICES  1500
#define MAX_SPRITE_COMMANDS 4096

typedef struct
{
  wt_vec2f_t pos;
  wt_color_t color;
} solid_vertex_t;

typedef struct
{
  struct
  {
    gpu_buffer_t vertex_buffer, index_buffer;
    gpu_shader_t shader;
  } solid2d;
} ren_state_t;

static ren_state_t *get_state(void)
{
  return game_get_state()->modules.ren;
}

void ren_init(void)
{
  game_state_t *gs = game_get_state();
  ren_state_t *s = gs->modules.ren = mem_hunk_push(sizeof(ren_state_t));

  // === solid 2D geometry ===
  {
    s->solid2d.vertex_buffer = gpu_buffer_new(&(gpu_buffer_desc_t){

      });
    s->solid2d.index_buffer = gpu_buffer_new(&(gpu_buffer_desc_t){

      });
    s->solid2d.shader = gpu_shader_new(&(gpu_shader_desc_t){
        .vs_path = "data/shaders/dx11/solid2d.vs.cso",
        .ps_path = "data/shaders/dx11/solid2d.ps.cso",
        .inputs[0] = { .format =
      });
  }
}

void ren_frame_begin(wt_vec2_t window_size)
{
  WT_UNUSED(window_size);

  gpu_frame_begin();
}

void ren_frame_end(void)
{
  gpu_frame_end();
}
