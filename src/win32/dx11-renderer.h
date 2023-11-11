#ifndef DX11_RENDERER_H
#define DX11_RENDERER_H

#define COBJMACROS
#include <d3d11.h>
#include <dxgi.h>
#include <wt/wt.h>
#include "../renderer.h"
#include "../system.h"

#define MAX_SOLID_VERTICES 1000
#define MAX_SOLID_INDICES  1500
#define MAX_SPRITE_COMMANDS 4096

#define CHUNK_DATA_ARENA_SIZE WT_MEGABYTES(32)

typedef struct
{
  ID3D11VertexShader *vs;
  ID3D11PixelShader *ps;
  ID3D11InputLayout *input_layout;
} dx11_shader_t;

typedef struct
{
  wt_vec2f_t rc_window_size;
  char pad[8];
  wt_mat4x4_t projection;
  wt_mat4x4_t view;
  wt_mat4x4_t model;
} dx11_cbuffer_t;

typedef enum
{
  BATCH_SOLID,
  BATCH_SPRITE,
} dx11_batch_type_t;

typedef struct
{
  wt_vec2f_t pos;
  wt_color_t color;
} dx11_solid_vertex_t;

typedef struct
{
  ID3D11Buffer *vertex_buffer, *index_buffer;
  dx11_solid_vertex_t vertices[MAX_SOLID_VERTICES];
  u16 indices[MAX_SOLID_INDICES];
  usize num_vertices, num_indices;
  dx11_shader_t shader;
} dx11_solid_batch_t;

typedef struct
{
  wt_recti_t src;
  wt_recti_t dst;
  f32 rotation;
} dx11_sprite_cmd_t;

typedef struct
{
  wt_vec2f_t rc_atlas_size;
} dx11_sprite_cbuffer_data_t;

typedef struct
{
  ID3D11Buffer *cmd_buffer;
  ID3D11ShaderResourceView *cmd_srv;
  ID3D11Buffer *const_buffer;
  dx11_sprite_cmd_t commands[MAX_SPRITE_COMMANDS];
  usize num_commands;
  ren_texture_t texture;
} dx11_sprite_batch_t;

typedef struct
{
  wt_vec2f_t position;
  wt_vec2f_t rc_atlas_size;
} dx11_chunk_cbuffer_data_t;

// an insanely fucking stupid hack to reduce memory usage
// basically, the amount of memory a chunk mesh uses can vary greatly, meaning we don't
// want to allocate the maximum upfront (causes a 16*16 area of chunks to eat up 4gb)
// however, d3d11 has no system to resize buffers at runtime
// so, we literally just create a new buffer and destroy the old one when we run out of space
// it's a good thing i can't be fired from something i do for fun
typedef struct
{
  ID3D11Buffer *buffer;
  usize capacity;
  int bind_flags;
} dx11_dynamic_buffer_t;

// format:
// 5 bits - x
// 5 bits - z
// 8 bits - y
// 8 bits - block
// 2 bits - texcoord index
// 4 bits - light level
typedef u32 dx11_chunk_vertex_t;

struct ren_chunk_t
{
  //ID3D11Buffer *vertex_buffer, *index_buffer;
  dx11_dynamic_buffer_t vertex_buffer, index_buffer;
  ID3D11Buffer *const_buffer;
  usize num_vertices, num_indices;
  wt_vec2_t position;
};

typedef struct
{
  void *buffer;
  u64 num_bytes;
  bool is_dynamic_buffer;
} dx11_chunk_data_footer_t;

typedef struct
{
  ID3D11Device *device;
  ID3D11DeviceContext *device_context;
  ID3D11RenderTargetView *render_target_view;
  IDXGISwapChain *swap_chain;

  ID3D11SamplerState *sampler_state;
  ID3D11BlendState *blend_state;
  ID3D11DepthStencilState *depth_stencil_state;
  ID3D11DepthStencilView *depth_stencil_view;

  // producer-consumer pattern - worker threads push chunk vertex/index data to this thread,
  // main thread updates the GPU buffers
  // producer pushes the data followed by a footer
  wt_arena_t chunk_data_arena;
  sys_mutex_t chunk_data_mutex;

  dx11_batch_type_t batch_type;
  wt_rect_t batch_bounds;

  ID3D11Buffer *const_buffer;

  dx11_shader_t sprite_shader;
  dx11_shader_t chunk_shader;
  ren_texture_t tiles_texture;
  
  dx11_solid_batch_t solid;
  dx11_sprite_batch_t sprites;

  wt_pool_t chunk_pool;

  wt_mat4x4_t view_matrix;

  wt_vec2_t window_size;
} ren_state_t;

dx11_shader_t       dx11_shader_load(const char *vs_path, const char *ps_path,
                      D3D11_INPUT_ELEMENT_DESC *inputs, usize num_inputs);
void                dx11_shader_bind(dx11_shader_t shd);

dx11_sprite_batch_t dx11_sprite_batch_new(void);
void                dx11_sprite_batch_flush(dx11_sprite_batch_t *sb);
void                dx11_sprite_batch_add_sprite(dx11_sprite_batch_t *sb, ren_texture_t tx,
                      wt_recti_t src, wt_recti_t dst, f32 rotation);

dx11_dynamic_buffer_t dx11_dynamic_buffer_new(int bind_flags);
void                  dx11_dynamic_buffer_update(dx11_dynamic_buffer_t *db, void *data, usize num_bytes);

#endif
