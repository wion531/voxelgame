#ifndef GPU_H
#define GPU_H

#define GPU_MAX_BUFFERS 16384
#define GPU_MAX_SHADERS 64
#define GPU_MAX_TEXTURES 32

#define GPU_SHADER_MAX_INPUTS 8

#include <wt/wt.h>

typedef void* gpu_buffer_t;
typedef void* gpu_texture_t;
typedef void* gpu_shader_t;
typedef void* gpu_framebuffer_t;

typedef enum
{
  GPU_INDEX_INVALID,
  GPU_INDEX_U8,
  GPU_INDEX_U16,
  GPU_INDEX_U32,
} gpu_index_type_t;

typedef enum
{
  GPU_DATA_NULL,
  GPU_DATA_INT,
  GPU_DATA_INT2,
  GPU_DATA_INT3,
  GPU_DATA_INT4,
  GPU_DATA_UINT,
  GPU_DATA_UINT2,
  GPU_DATA_UINT3,
  GPU_DATA_UINT4,
  GPU_DATA_FLOAT,
  GPU_DATA_FLOAT2,
  GPU_DATA_FLOAT3,
  GPU_DATA_FLOAT4,
  GPU_DATA_BYTE4_NORM,
} gpu_data_type_t;

typedef enum
{
  GPU_SHADER_STAGE_VERTEX,
  GPU_SHADER_STAGE_PIXEL,
} gpu_shader_stage_t;

typedef enum
{
  GPU_BUFFER_VERTEX,
  GPU_BUFFER_INDEX,
  GPU_BUFFER_STRUCTS,
  GPU_BUFFER_CONSTANT,
} gpu_buffer_type_t;

typedef struct
{
  gpu_buffer_type_t type;
  usize stride;
  bool dynamic; // can this buffer be updated?
  void *data; // can be NULL to create an empty buffer
  usize size;
  gpu_index_type_t index_type;
  gpu_shader_stage_t shader_stage;
} gpu_buffer_desc_t;

typedef struct
{
  int width, height;
  wt_color_t *pixels;
  bool dynamic;
} gpu_texture_desc_t;

typedef struct
{
  gpu_data_type_t type;
  usize buffer_index;
} gpu_shader_input_t;

typedef struct
{
  const char *vs_path;
  const char *ps_path;
  gpu_shader_input_t inputs[GPU_SHADER_MAX_INPUTS];
} gpu_shader_desc_t;

typedef enum
{
  GPU_PRIMITIVE_POINTS,
  GPU_PRIMITIVE_LINES,
  GPU_PRIMITIVE_LINE_STRIP,
  GPU_PRIMITIVE_TRIANGLES,
  GPU_PRIMITIVE_TRIANGLE_STRIP,
} gpu_primitive_type_t;

void gpu_init(void);
void gpu_frame_begin(void);
void gpu_frame_end(void);

gpu_buffer_t gpu_buffer_new(gpu_buffer_desc_t *desc);
void         gpu_buffer_update(gpu_buffer_t buf, void *data, usize size);
void         gpu_buffer_bind(gpu_buffer_t buf, u32 slot);
void         gpu_buffer_free(gpu_buffer_t buf);

gpu_texture_t gpu_texture_new(gpu_texture_desc_t *desc);
wt_vec2_t     gpu_texture_get_size(gpu_texture_t tex);
void          gpu_texture_update(gpu_texture_t tex, wt_color_t *data, usize size_bytes);
void          gpu_texture_bind(gpu_texture_t tex, u32 slot);
void          gpu_texture_free(gpu_texture_t tex);

// todo: create a universal shader format for this
gpu_shader_t gpu_shader_new(gpu_shader_desc_t *desc);
void         gpu_shader_bind(gpu_shader_t shd);
void         gpu_shader_free(gpu_shader_t shd);

gpu_framebuffer_t gpu_framebuffer_new(wt_vec2_t size);
wt_vec2_t         gpu_framebuffer_get_size(gpu_framebuffer_t fb);
gpu_texture_t     gpu_framebuffer_get_texture(gpu_framebuffer_t fb);
void              gpu_framebuffer_bind(gpu_framebuffer_t fb);
void              gpu_framebuffer_unbind(gpu_framebuffer_t fb);
void              gpu_framebuffer_free(gpu_framebuffer_t fb);

void gpu_clear(wt_color_t clr);
void gpu_draw(gpu_primitive_type_t pt, usize offset, usize num_vertices);
void gpu_draw_indexed(gpu_primitive_type_t pt, usize offset, usize num_indices);
void gpu_draw_instanced(gpu_primitive_type_t pt, usize offset, usize num_vertices, usize num_instances);
void gpu_draw_indexed_instanced(gpu_primitive_type_t pt, usize offset, usize num_indices, usize num_instances);

#endif
