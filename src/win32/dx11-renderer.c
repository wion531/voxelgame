#define STB_IMAGE_IMPLEMENTATION
#include "dx11-renderer.h"
#include "../constants.h"
#include "../game.h"
#include "../memory.h"
#include "../system.h"
#include <string.h>
#include <stb_image.h>

#define CHECK(expr) do {\
    HRESULT hr_ = expr;\
    WT_UNUSED(hr_);\
    if (FAILED(hr_))\
      printf("error code: %lx\n", hr_);\
    WT_ASSERT(SUCCEEDED(hr_) && #expr " failed");\
  } while (0)

static ren_state_t *get_state(void)
{
  return game_get_state()->modules.ren;
}

static void safe_release(void *obj)
{
  if (obj)
  {
    ID3D11Resource *rsrc = (ID3D11Resource*)obj;
    rsrc->lpVtbl->Release(rsrc);
  }
}

static void update_buffer(ID3D11Buffer *buffer, void *data, usize num_bytes)
{
  ren_state_t *s = get_state();
  D3D11_MAPPED_SUBRESOURCE mapped = { 0 };

  ID3D11DeviceContext_Map(s->device_context, (ID3D11Resource*)buffer, 0,
    D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  WT_ASSERT(mapped.pData);
  memcpy(mapped.pData, data, num_bytes);
  ID3D11DeviceContext_Unmap(s->device_context, (ID3D11Resource*)buffer, 0);
}

dx11_shader_t dx11_shader_load(const char *vs_path, const char *ps_path,
  D3D11_INPUT_ELEMENT_DESC *inputs, usize num_inputs)
{
  ren_state_t *s = get_state();
  dx11_shader_t res = { 0 };
  mem_scratch_begin();

  sys_file_contents_t vs_contents = sys_file_read_to_scratch_buffer(vs_path, false);
  sys_file_contents_t ps_contents = sys_file_read_to_scratch_buffer(ps_path, false);

  if (vs_contents.data && ps_contents.data)
  {
    CHECK(ID3D11Device_CreateVertexShader(s->device, vs_contents.data, vs_contents.size, NULL, &res.vs));
    CHECK(ID3D11Device_CreatePixelShader(s->device, ps_contents.data, ps_contents.size, NULL, &res.ps));

    if (inputs && num_inputs > 0)
    {
      CHECK(ID3D11Device_CreateInputLayout(s->device, inputs, num_inputs,
        vs_contents.data, vs_contents.size, &res.input_layout));
    }
  }

  mem_scratch_end();
  return res;
}

void dx11_shader_bind(dx11_shader_t shd)
{
  ren_state_t *s = get_state();
  ID3D11DeviceContext_VSSetShader(s->device_context, shd.vs, NULL, 0);
  ID3D11DeviceContext_PSSetShader(s->device_context, shd.ps, NULL, 0);
  ID3D11DeviceContext_IASetInputLayout(s->device_context, shd.input_layout);
}

dx11_sprite_batch_t dx11_sprite_batch_new(void)
{
  ren_state_t *s = get_state();
  dx11_sprite_batch_t res = { 0 };
  CHECK(ID3D11Device_CreateBuffer(s->device, (&(D3D11_BUFFER_DESC){
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .ByteWidth = MAX_SPRITE_COMMANDS * sizeof(dx11_sprite_cmd_t),
      .Usage = D3D11_USAGE_DYNAMIC,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      .MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
      .StructureByteStride = sizeof(dx11_sprite_cmd_t),
    }), NULL, &res.cmd_buffer));
  CHECK(ID3D11Device_CreateShaderResourceView(s->device, (ID3D11Resource*)res.cmd_buffer,
    (&(D3D11_SHADER_RESOURCE_VIEW_DESC){
      .Format = DXGI_FORMAT_UNKNOWN,
      .ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
      .Buffer.NumElements = MAX_SPRITE_COMMANDS,
    }), &res.cmd_srv));
  CHECK(ID3D11Device_CreateBuffer(s->device, (&(D3D11_BUFFER_DESC){
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .ByteWidth = (sizeof(dx11_sprite_cbuffer_data_t) + 0xf) & ~0xf,
      .Usage = D3D11_USAGE_DYNAMIC,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    }), NULL, &res.const_buffer));

  return res;
}

void dx11_sprite_batch_flush(dx11_sprite_batch_t *sb)
{
  ren_state_t *s = get_state();
  if (sb->num_commands > 0)
  {
    update_buffer(sb->cmd_buffer, sb->commands, sb->num_commands * sizeof(dx11_sprite_cmd_t));
    ID3D11DeviceContext_IASetPrimitiveTopology(s->device_context,
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    ID3D11Buffer *const_buffers[2] = { s->const_buffer, sb->const_buffer };
    ID3D11DeviceContext_IASetVertexBuffers(s->device_context, 0, 0, NULL, NULL, NULL);
    ID3D11DeviceContext_IASetIndexBuffer(s->device_context, NULL, DXGI_FORMAT_UNKNOWN, 0);
    ID3D11DeviceContext_VSSetShaderResources(s->device_context, 0, 1, &sb->cmd_srv);
    ID3D11DeviceContext_VSSetConstantBuffers(s->device_context, 0, 2, const_buffers);
    ID3D11DeviceContext_PSSetShaderResources(s->device_context, 1, 1,
      (ID3D11ShaderResourceView**)&sb->texture.data);
    ID3D11DeviceContext_PSSetSamplers(s->device_context, 0, 1, &s->sampler_state);
    dx11_shader_bind(s->sprite_shader);
    ID3D11DeviceContext_DrawInstanced(s->device_context, 4, sb->num_commands, 0, 0);

    sb->num_commands = 0;
  }
}

#if 0
static bool rects_intersect(wt_rect_t a, wt_rect_t b)
{
  return (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y);
}
#endif

// expand the current batch's bounds rectangle to accomodate for
// the given rectangle
static void expand_bounds_to_fit(wt_rect_t rect)
{
  ren_state_t *s = get_state();

  i32 bmnx = s->batch_bounds.x;
  i32 bmny = s->batch_bounds.y;
  i32 bmxx = s->batch_bounds.x + s->batch_bounds.w;
  i32 bmxy = s->batch_bounds.y + s->batch_bounds.h;

  // the batch's bounds is zero if there's nothing in it yet
  if (s->batch_bounds.x == WT_I32_MAX)
  {
    bmxx = 0;
    bmxy = 0;
  }

  i32 rmnx = rect.x;
  i32 rmny = rect.y;
  i32 rmxx = rect.x + rect.w;
  i32 rmxy = rect.y + rect.h;
  i32 mnx = WT_MIN(bmnx, rmnx);
  i32 mny = WT_MIN(bmny, rmny);
  i32 mxx = WT_MAX(bmxx, rmxx);
  i32 mxy = WT_MAX(bmxy, rmxy);

  s->batch_bounds = wt_rect(mnx, mny, mxx - mnx, mxy - mny);
}

static void flush(void)
{
  ren_state_t *s = get_state();
  switch (s->batch_type)
  {
  case BATCH_SOLID:
  {
    update_buffer(s->solid.vertex_buffer, s->solid.vertices,
      s->solid.num_vertices * sizeof(dx11_solid_vertex_t));
    update_buffer(s->solid.index_buffer, s->solid.indices, s->solid.num_indices * sizeof(u16));

    const UINT stride = sizeof(dx11_solid_vertex_t), offset = 0;

    dx11_shader_bind(s->solid.shader);
    ID3D11DeviceContext_IASetVertexBuffers(s->device_context, 0, 1, &s->solid.vertex_buffer,
      &stride, &offset);
    ID3D11DeviceContext_IASetIndexBuffer(s->device_context, s->solid.index_buffer,
      DXGI_FORMAT_R16_UINT, 0);
    ID3D11DeviceContext_IASetPrimitiveTopology(s->device_context,
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetConstantBuffers(s->device_context, 0, 1, &s->const_buffer);
    ID3D11DeviceContext_DrawIndexed(s->device_context, s->solid.num_indices, 0, 0);

    s->solid.num_vertices = s->solid.num_indices = 0;
    break;
  }
  case BATCH_SPRITE:
  {
    dx11_sprite_batch_flush(&s->sprites);
    break;
  }
  default: break;
  }

  s->batch_bounds = wt_rect(WT_I32_MAX, WT_I32_MAX, 0, 0);
}

static void begin_primitive(dx11_batch_type_t type, wt_rect_t bounds)
{
  ren_state_t *s = get_state();

  expand_bounds_to_fit(bounds);

  // if the batch type is the same, we're good to go
  if (s->batch_type == type)
  {
    return;
  }
  // otherwise, flush existing batch
  else
  {
    flush();
    s->batch_type = type;
  }
}

void dx11_sprite_batch_add_sprite(dx11_sprite_batch_t *sb, ren_texture_t tx,
  wt_recti_t src, wt_recti_t dst, f32 rotation)
{
  if (sb->num_commands + 1 > MAX_SPRITE_COMMANDS)
  {
    dx11_sprite_batch_flush(sb);
  }

  if (sb->texture.data != tx.data)
  {
    dx11_sprite_batch_flush(sb);

    sb->texture = tx;
    dx11_sprite_cbuffer_data_t cbuffer_data = { 0 };
    cbuffer_data.rc_atlas_size = wt_vec2f_div(wt_vec2f(1.0f, 1.0f), wt_vec2f(tx.width, tx.height));
    update_buffer(sb->const_buffer, &cbuffer_data, sizeof(cbuffer_data));
  }

  begin_primitive(BATCH_SPRITE, dst);

  dx11_sprite_cmd_t *cmd = sb->commands + sb->num_commands++;
  cmd->dst = dst;
  cmd->src = src;
  cmd->rotation = rotation;
}

dx11_dynamic_buffer_t dx11_dynamic_buffer_new(int bind_flags)
{
  dx11_dynamic_buffer_t res = { 0 };
  res.bind_flags = bind_flags;
  res.capacity = 1024;
  return res;
}

void dx11_dynamic_buffer_update(dx11_dynamic_buffer_t *db, void *data, usize num_bytes)
{
  ren_state_t *s = get_state();

  if (num_bytes == 0) return;

  if (db->buffer == NULL || db->capacity < num_bytes)
  {
#if 0
    while (db->capacity < num_bytes)
    {
      db->capacity *= 1.5;
    }
#else
    db->capacity = (num_bytes + 0xfff) & ~0xfff;
#endif

    ID3D11Buffer *old = db->buffer;
    CHECK(ID3D11Device_CreateBuffer(s->device, (&(D3D11_BUFFER_DESC){
        .ByteWidth = db->capacity,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = db->bind_flags,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      }), NULL, &db->buffer));
    safe_release(old);
  }
  update_buffer(db->buffer, data, num_bytes);
}

void ren_init(void)
{
  game_state_t *gs = game_get_state();
  ren_state_t *s = gs->modules.ren = mem_hunk_push(sizeof(ren_state_t));

  extern HWND sys_win32_get_hwnd(void);

  // === device and swap chain ===
  int device_flags = 0;
#if WT_DEBUG
  device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  CHECK(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, device_flags,
    NULL, 0, D3D11_SDK_VERSION, &(DXGI_SWAP_CHAIN_DESC){
      .BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .SampleDesc.Count = 1,
      .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
      .BufferCount = 2,
      .OutputWindow = sys_win32_get_hwnd(),
      .Windowed = true,
      .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    }, &s->swap_chain, &s->device, NULL, &s->device_context));

  // === constant buffer ===
  CHECK(ID3D11Device_CreateBuffer(s->device, (&(D3D11_BUFFER_DESC){
      .ByteWidth = (sizeof(dx11_cbuffer_t) + 0xf) & ~0xf,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    }), NULL, &s->const_buffer));

  // === sampler state ===
  CHECK(ID3D11Device_CreateSamplerState(s->device, (&(D3D11_SAMPLER_DESC){
      .AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
      .AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
      .Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
      .ComparisonFunc = D3D11_COMPARISON_NEVER,
    }), &s->sampler_state));

  // === blend state ===
  CHECK(ID3D11Device_CreateBlendState(s->device, (&(D3D11_BLEND_DESC){
      .RenderTarget[0].BlendEnable = true,
      .RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA,
      .RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
      .RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD,
      .RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
      .RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
      .RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD,
      .RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
    }), &s->blend_state));

  // === depth stencil state ===
  CHECK(ID3D11Device_CreateDepthStencilState(s->device, (&(D3D11_DEPTH_STENCIL_DESC){
      .DepthEnable = true,
      .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
      .DepthFunc = D3D11_COMPARISON_LESS_EQUAL,
    }), &s->depth_stencil_state));

  // === solid batch ===
  {
    D3D11_INPUT_ELEMENT_DESC inputs[] = {
      { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    s->solid.shader = dx11_shader_load("data/shaders/dx11/solid.vs.cso",
      "data/shaders/dx11/solid.ps.cso", inputs, WT_ARRAY_COUNT(inputs));

    CHECK(ID3D11Device_CreateBuffer(s->device, (&(D3D11_BUFFER_DESC){
        .ByteWidth = sizeof(s->solid.vertices),
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      }), NULL, &s->solid.vertex_buffer));
    CHECK(ID3D11Device_CreateBuffer(s->device, (&(D3D11_BUFFER_DESC){
        .ByteWidth = sizeof(s->solid.indices),
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_INDEX_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
      }), NULL, &s->solid.index_buffer));
  }

  // === sprite batch ===
  {
    s->sprite_shader = dx11_shader_load("data/shaders/dx11/sprite.vs.cso",
      "data/shaders/dx11/sprite.ps.cso", NULL, 0);
    s->sprites = dx11_sprite_batch_new();
  }

  // === chunks ===
  {
    void *pool_buffer = mem_hunk_push(CHUNK_MAX * sizeof(struct ren_chunk_t));
    s->chunk_pool = wt_pool_new(pool_buffer, CHUNK_MAX, sizeof(struct ren_chunk_t));

    D3D11_INPUT_ELEMENT_DESC inputs[] = {
      { "CUNT", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    s->chunk_shader = dx11_shader_load("data/shaders/dx11/chunk.vs.cso",
      "data/shaders/dx11/chunk.ps.cso", inputs, WT_ARRAY_COUNT(inputs));

    s->tiles_texture = ren_texture_load_from_file("data/tiles.png");
  }

  s->chunk_data_mutex = sys_mutex_new();
  s->chunk_data_arena = wt_arena_new(mem_hunk_push(CHUNK_DATA_ARENA_SIZE), CHUNK_DATA_ARENA_SIZE);
  s->batch_bounds = wt_rect(WT_I32_MAX, WT_I32_MAX, 0, 0);
}

static void draw_solid_primitive(dx11_solid_vertex_t *vertices, usize num_vertices,
  u16 *indices, usize num_indices)
{
  ren_state_t *s = get_state();
  wt_rect_t bounds = { vertices[0].pos.x, vertices[0].pos.y, 0, 0 };

  // determine the bounds of the given draw command
  for (usize i = 0; i < num_vertices; ++i)
  {
    dx11_solid_vertex_t v = vertices[i];
    if (v.pos.x < bounds.x) { bounds.x = v.pos.x; }
    if (v.pos.y < bounds.y) { bounds.y = v.pos.y; }
    if (v.pos.x > bounds.x + bounds.w) { bounds.w = v.pos.x - bounds.x; }
    if (v.pos.y > bounds.y + bounds.h) { bounds.h = v.pos.y - bounds.y; }
  }

  // flush if necessary
  if (s->solid.num_vertices + num_vertices > MAX_SOLID_VERTICES ||
    s->solid.num_indices + num_indices > MAX_SOLID_INDICES)
  {
    flush();
  }

  begin_primitive(BATCH_SOLID, bounds);

  memcpy(s->solid.vertices + s->solid.num_vertices, vertices,
    num_vertices * sizeof(dx11_solid_vertex_t));

  usize actual_num_indices = (num_indices == 0 ? num_vertices : num_indices);
  for (usize i = 0; i < actual_num_indices; ++i)
  {
    if (indices == NULL)
      s->solid.indices[s->solid.num_indices + i] = s->solid.num_vertices + i;
    else
      s->solid.indices[s->solid.num_indices + i] = s->solid.num_vertices + indices[i];
  }
  s->solid.num_vertices += num_vertices;
  s->solid.num_indices += actual_num_indices;
}

void ren_frame_begin(wt_vec2_t window_size)
{
  ren_state_t *s = get_state();
  if ((s->window_size.x != window_size.x || s->window_size.y != window_size.y) &&
    window_size.x > 0 && window_size.y > 0)
  {
    safe_release(s->render_target_view);
    safe_release(s->depth_stencil_view);
    ID3D11DeviceContext_ClearState(s->device_context);
    CHECK(IDXGISwapChain_ResizeBuffers(s->swap_chain, 0, window_size.x, window_size.y,
      DXGI_FORMAT_UNKNOWN, 0));

    ID3D11Texture2D *framebuffer = NULL;
    CHECK(IDXGISwapChain_GetBuffer(s->swap_chain, 0, &IID_ID3D11Texture2D, (void**)&framebuffer));
    CHECK(ID3D11Device_CreateRenderTargetView(s->device, (ID3D11Resource*)framebuffer, NULL,
      &s->render_target_view));
    safe_release(framebuffer);

    ID3D11Texture2D *depth_stencil_texture = NULL;
    CHECK(ID3D11Device_CreateTexture2D(s->device, (&(D3D11_TEXTURE2D_DESC){
        .Width = window_size.x,
        .Height = window_size.y,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc.Count = 1,
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_DEPTH_STENCIL,
      }), NULL, &depth_stencil_texture));
    CHECK(ID3D11Device_CreateDepthStencilView(s->device, (ID3D11Resource*)depth_stencil_texture,
      (&(D3D11_DEPTH_STENCIL_VIEW_DESC){
        .Format = DXGI_FORMAT_D32_FLOAT,
        .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D,
      }), &s->depth_stencil_view));
    //safe_release(depth_stencil_texture);

    s->window_size = window_size;
  }

  dx11_cbuffer_t cb = { 0 };
  cb.rc_window_size = wt_vec2f_div(wt_vec2f(1.0f, 1.0f), wt_vec2i_to_vec2f(window_size));
  cb.projection = wt_mat4x4_perspective(0.2f, (f32)window_size.x / (f32)window_size.y, 0.001f, 10000.0f);
  cb.view = s->view_matrix;
  update_buffer(s->const_buffer, &cb, sizeof(dx11_cbuffer_t));

  D3D11_VIEWPORT viewport = {
    0, 0, window_size.x, window_size.y, 0.0f, 1.0f
  };

  ID3D11DeviceContext_RSSetViewports(s->device_context, 1, &viewport);
  ID3D11DeviceContext_OMSetRenderTargets(s->device_context, 1, &s->render_target_view,
    s->depth_stencil_view);
  ID3D11DeviceContext_OMSetBlendState(s->device_context, s->blend_state, NULL, WT_U32_MAX);

  const float k_color[4] = { 0.55f, 0.82f, 0.93f, 1.0f };
  ID3D11DeviceContext_ClearRenderTargetView(s->device_context, s->render_target_view, k_color);
  ID3D11DeviceContext_ClearDepthStencilView(s->device_context, s->depth_stencil_view,
    D3D11_CLEAR_DEPTH, 1.0f, 0);
}

void ren_frame_end(void)
{
  ren_state_t *s = get_state();

  // update any chunk meshes received from the worker threads
  sys_mutex_lock(s->chunk_data_mutex);
  while (s->chunk_data_arena.pos > 0)
  {
    dx11_chunk_data_footer_t footer = { 0 };
    memcpy(&footer, wt_arena_get_last(&s->chunk_data_arena, sizeof(footer)), sizeof(footer));
    wt_arena_pop(&s->chunk_data_arena, sizeof(footer));

    void *data = wt_arena_get_last(&s->chunk_data_arena, footer.num_bytes);
    if (footer.is_dynamic_buffer)
    {
      dx11_dynamic_buffer_update(footer.buffer, data, footer.num_bytes);
    }
    else
    {
      update_buffer(footer.buffer, data, footer.num_bytes);
    }

    wt_arena_pop(&s->chunk_data_arena, footer.num_bytes);
  }

  sys_mutex_unlock(s->chunk_data_mutex);

  flush();
  IDXGISwapChain_Present(s->swap_chain, 1, 0);
}

ren_texture_t ren_texture_load_from_file(const char *filename)
{
  int width, height;
  byte_t *pixels = stbi_load(filename, &width, &height, NULL, 4);
  if (pixels)
  {
    ren_texture_t res = ren_texture_new((wt_color_t*)pixels, width, height);
    stbi_image_free(pixels);
    return res;
  }
  return (ren_texture_t){ 0 };
}

ren_texture_t ren_texture_new(wt_color_t *pixels, int width, int height)
{
  ren_state_t *s = get_state();
  ren_texture_t res = { 0 };
  ID3D11Texture2D *texture = NULL;

  res.width = width;
  res.height = height;

  CHECK(ID3D11Device_CreateTexture2D(s->device, (&(D3D11_TEXTURE2D_DESC){
      .Width = width,
      .Height = height,
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .SampleDesc.Count = 1,
      .Usage = D3D11_USAGE_IMMUTABLE,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
    }), (&(D3D11_SUBRESOURCE_DATA){
      .pSysMem = pixels,
      .SysMemPitch = width * 4,
    }), &texture));
  CHECK(ID3D11Device_CreateShaderResourceView(s->device, (ID3D11Resource*)texture, NULL,
    (ID3D11ShaderResourceView**)&res.data));
  safe_release((ID3D11Resource*)texture);

  return res;
}

void ren_texture_free(ren_texture_t tx)
{
  safe_release((ID3D11Resource*)tx.data);
}

ren_chunk_t ren_chunk_new(wt_vec2_t pos)
{
  ren_state_t *s = get_state();
  ren_chunk_t res = wt_pool_alloc(&s->chunk_pool);
  res->position = pos;

  res->vertex_buffer = dx11_dynamic_buffer_new(D3D11_BIND_VERTEX_BUFFER);
  res->index_buffer = dx11_dynamic_buffer_new(D3D11_BIND_INDEX_BUFFER);

  CHECK(ID3D11Device_CreateBuffer(s->device, (&(D3D11_BUFFER_DESC){
      .ByteWidth = (sizeof(dx11_chunk_cbuffer_data_t) + 0xf) & ~0xf,
      .Usage = D3D11_USAGE_DYNAMIC,
      .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
      .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    }), NULL, &res->const_buffer));

  return res;
}

block_id_t world_get_block(wt_vec3_t pos);

// todo: this function could probably be made portable
void ren_chunk_generate_mesh(ren_chunk_t c, block_id_t *blocks)
{
  ren_state_t *s = get_state();
  mem_scratch_begin();

  usize vertices_num_bytes = sizeof(dx11_chunk_vertex_t) * 4 * 6 * CHUNK_NUM_BLOCKS;
  usize indices_num_bytes = sizeof(u32) * 6 * 6 * CHUNK_NUM_BLOCKS;

  dx11_chunk_vertex_t *vertices = mem_scratch_push(vertices_num_bytes);
  u32 *indices = mem_scratch_push(indices_num_bytes);

  usize num_vertices = 0;
  usize num_indices = 0;

  bool shadows[CHUNK_SIZE_X * CHUNK_SIZE_Z] = { 0 };

//  for (usize i = 0; i < CHUNK_NUM_BLOCKS; ++i)
  for (isize i = CHUNK_NUM_BLOCKS - 1; i >= 0; --i)
  {
    if (blocks[i] == 0)
    {
      continue;
    }

    block_info_t *info = block_get_info(blocks[i]);
    WT_ASSERT(info);

    u32 top_idx = info->atlas_tiles[0].x + info->atlas_tiles[0].y * 16;
    u32 bot_idx = info->atlas_tiles[1].x + info->atlas_tiles[1].y * 16;
    u32 fnt_idx = info->atlas_tiles[2].x + info->atlas_tiles[2].y * 16;
    u32 bck_idx = info->atlas_tiles[3].x + info->atlas_tiles[3].y * 16;
    u32 lft_idx = info->atlas_tiles[4].x + info->atlas_tiles[4].y * 16;
    u32 rgt_idx = info->atlas_tiles[5].x + info->atlas_tiles[5].y * 16;

    typedef struct { wt_vec3_t pos; u32 block; u32 texcoord; u32 light; } unpacked_vertex_t;

    u8 bri = 15;
    u8 mid = 8;
    u8 mid2 = 9;
    u8 dim = 4;

    unpacked_vertex_t block_vertices[] = {
      // top face
      { { 0, 1, 0 }, top_idx, 0, bri },
      { { 1, 1, 0 }, top_idx, 1, bri },
      { { 0, 1, 1 }, top_idx, 2, bri },
      { { 1, 1, 1 }, top_idx, 3, bri },

      // bottom face
      { { 0, 0, 0 }, bot_idx, 0, dim },
      { { 1, 0, 0 }, bot_idx, 1, dim },
      { { 0, 0, 1 }, bot_idx, 2, dim },
      { { 1, 0, 1 }, bot_idx, 3, dim },

      // front face
      { { 0, 0, 0 }, fnt_idx, 2, mid },
      { { 1, 0, 0 }, fnt_idx, 3, mid },
      { { 0, 1, 0 }, fnt_idx, 0, mid },
      { { 1, 1, 0 }, fnt_idx, 1, mid },

      // back face
      { { 0, 0, 1 }, bck_idx, 2, mid },
      { { 1, 0, 1 }, bck_idx, 3, mid },
      { { 0, 1, 1 }, bck_idx, 0, mid },
      { { 1, 1, 1 }, bck_idx, 1, mid },

      // left face
      { { 0, 0, 0 }, lft_idx, 3, mid2 },
      { { 0, 1, 0 }, lft_idx, 1, mid2 },
      { { 0, 0, 1 }, lft_idx, 2, mid2 },
      { { 0, 1, 1 }, lft_idx, 0, mid2 },

      // right face
      { { 1, 0, 0 }, rgt_idx, 2, mid2 },
      { { 1, 1, 0 }, rgt_idx, 0, mid2 },
      { { 1, 0, 1 }, rgt_idx, 3, mid2 },
      { { 1, 1, 1 }, rgt_idx, 1, mid2 },
    };
    u32 block_indices[] = {
      0, 1, 3,
      3, 2, 0,

      3, 1, 0,
      0, 2, 3,

      0, 1, 3,
      3, 2, 0,

      3, 1, 0,
      0, 2, 3,

      0, 1, 3,
      3, 2, 0,

      3, 1, 0,
      0, 2, 3,
    };

    f32 fi = i;
    wt_vec3f_t block_coords = wt_vec3f(
      fmodf(fi, CHUNK_SIZE_X),
      fi / (CHUNK_SIZE_X * CHUNK_SIZE_Z),
      fmodf((fi / CHUNK_SIZE_X), CHUNK_SIZE_Z));

    wt_vec3_t neighbors[] = {
      {  0,  1,  0 },
      {  0, -1,  0 },
      {  0,  0, -1 },
      {  0,  0,  1 },
      { -1,  0,  0 },
      {  1,  0,  0 },
    };

    bool *shadow = &shadows[(int)block_coords.x + (int)block_coords.z * CHUNK_SIZE_X];
    if (*shadow)
    {
      block_vertices[0].light = WT_MAX(0, (int)block_vertices[0].light - 5);
      block_vertices[1].light = WT_MAX(0, (int)block_vertices[1].light - 5);
      block_vertices[2].light = WT_MAX(0, (int)block_vertices[2].light - 5);
      block_vertices[3].light = WT_MAX(0, (int)block_vertices[3].light - 5);
    }
    *shadow = true;

    for (usize j = 0; j < 6; ++j)
    {
      wt_vec3_t neighbor_offset = neighbors[j];
      wt_vec3_t neighbor = wt_vec3i_add(wt_vec3f_to_vec3i(block_coords), neighbor_offset);

      // if the block has a solid neighbor, we can skip drawing this face
      bool out_of_bounds =
        (neighbor.x < 0 || neighbor.x >= CHUNK_SIZE_X) ||
        (neighbor.y < 0 || neighbor.y >= CHUNK_SIZE_Y) ||
        (neighbor.z < 0 || neighbor.z >= CHUNK_SIZE_Z);

      block_id_t neighbor_id = 0;
      if (neighbor.y >= 0)
      {
        if (out_of_bounds)
        {
          wt_vec2_t chunk_pos = wt_vec2i_mul_i32(c->position, 16);
          neighbor.x += chunk_pos.x;
          neighbor.z += chunk_pos.y;
          neighbor_id = world_get_block(neighbor);
        }
        else
        {
          neighbor_id = blocks[neighbor.x + (neighbor.z * CHUNK_SIZE_Z) +
            (neighbor.y * CHUNK_SIZE_X * CHUNK_SIZE_Z)];
        }
      }

      if (neighbor_id != 0)
      {
        block_info_t *neighbor_info = block_get_info(neighbor_id);
        if (!neighbor_info || neighbor_info->solid)
        {
          continue;
        }
      }

      for (usize k = 0; k < 6; ++k)
      {
        indices[num_indices++] = block_indices[j * 6 + k] + num_vertices;
      }

      for (usize k = 0; k < 4; ++k)
      {
        unpacked_vertex_t u = block_vertices[j * 4 + k];
        dx11_chunk_vertex_t p = 0;
        u.pos = wt_vec3i_add(u.pos, wt_vec3f_to_vec3i(block_coords));
        p |= u.pos.x    << (32 - 5);
        p |= u.pos.z    << (32 - 10);
        p |= u.pos.y    << (32 - 18);
        p |= u.block    << (32 - 26);
        p |= u.texcoord << (32 - 28);
        p |= u.light    << (32 - 32);
        vertices[num_vertices++] = p;
      }
    }
  }

  dx11_chunk_cbuffer_data_t cbuffer_data = { 0 };
  cbuffer_data.position = wt_vec2f(c->position.x * CHUNK_SIZE_X, c->position.y * CHUNK_SIZE_Z);
  cbuffer_data.rc_atlas_size = wt_vec2f_div(wt_vec2f(1.0f, 1.0f), wt_vec2f(256.0f, 256.0f));

  // send buffer data to main thread via arena
  sys_mutex_lock(s->chunk_data_mutex);
  {
    dx11_chunk_data_footer_t footer = { 0 };

  //update_buffer(c->const_buffer, &cbuffer_data, sizeof(cbuffer_data));
    footer.buffer = c->const_buffer;
    footer.num_bytes = sizeof(cbuffer_data);
    wt_arena_push_from(&s->chunk_data_arena, &cbuffer_data, sizeof(cbuffer_data));
    wt_arena_push_from(&s->chunk_data_arena, &footer, sizeof(footer));

    footer.buffer = &c->vertex_buffer;
    footer.num_bytes = num_vertices * sizeof(dx11_chunk_vertex_t);
    footer.is_dynamic_buffer = true;
    wt_arena_push_from(&s->chunk_data_arena, vertices, footer.num_bytes);
    wt_arena_push_from(&s->chunk_data_arena, &footer, sizeof(footer));

    footer.buffer = &c->index_buffer;
    footer.num_bytes = num_indices * sizeof(u32);
    footer.is_dynamic_buffer = true;
    wt_arena_push_from(&s->chunk_data_arena, indices, footer.num_bytes);
    wt_arena_push_from(&s->chunk_data_arena, &footer, sizeof(footer));

    sys_mutex_unlock(s->chunk_data_mutex);
  }

  c->num_vertices = num_vertices;
  c->num_indices = num_indices;

  mem_scratch_end();
}

void ren_chunk_free(ren_chunk_t c)
{
  ren_state_t *s = get_state();
  wt_pool_free(&s->chunk_pool, c);
  safe_release((ID3D11Resource*)c->vertex_buffer.buffer);
  safe_release((ID3D11Resource*)c->index_buffer.buffer);
}

void ren_camera_set(wt_mat4x4_t mtx)
{
  ren_state_t *s = get_state();
  s->view_matrix = mtx;
}

void ren_draw_box(wt_rect_t rect, wt_color_t color)
{
  ren_draw_rect(wt_rect(rect.x,              rect.y,              rect.w, 1     ), color);
  ren_draw_rect(wt_rect(rect.x,              rect.y,              1,      rect.h), color);
  ren_draw_rect(wt_rect(rect.x,              rect.y + rect.h - 1, rect.w, 1     ), color);
  ren_draw_rect(wt_rect(rect.x + rect.w - 1, rect.y,              1,      rect.h), color);
}

void ren_draw_rect(wt_rect_t rect, wt_color_t color)
{
  dx11_solid_vertex_t vertices[] = {
    { { rect.x,          rect.y          }, color },
    { { rect.x + rect.w, rect.y          }, color },
    { { rect.x,          rect.y + rect.h }, color },
    { { rect.x + rect.w, rect.y + rect.h }, color },
  };
  u16 indices[] = { 0, 1, 3, 3, 2, 0 };
  draw_solid_primitive(vertices, WT_ARRAY_COUNT(vertices), indices, WT_ARRAY_COUNT(indices));
}

void ren_draw_sprite(ren_texture_t tx, wt_vec2_t pos)
{
  ren_draw_sprite_scaled(tx, pos, 1);
}

void ren_draw_sprite_scaled(ren_texture_t tx, wt_vec2_t pos, i32 scale)
{
  ren_draw_sprite_stretch(tx, wt_rect(0, 0, tx.width, tx.height),
    wt_rect(pos.x, pos.y, tx.width * scale, tx.height * scale));
}

void ren_draw_sprite_stretch(ren_texture_t tx, wt_rect_t src, wt_rect_t dst)
{
  ren_draw_sprite_ex(tx, src, dst, wt_vec2(0, 0), 0.0f, wt_color(255, 255, 255, 255));
}

void ren_draw_sprite_ex(ren_texture_t tx, wt_rect_t src, wt_rect_t dst,
  wt_vec2_t origin, float rotation, wt_color_t tint)
{
  WT_UNUSED(origin);
  WT_UNUSED(rotation);
  WT_UNUSED(tint);

  ren_state_t *s = get_state();
  dx11_sprite_batch_add_sprite(&s->sprites, tx, src, dst, rotation);
}

void ren_draw_chunk(ren_chunk_t c)
{
  ren_state_t *s = get_state();

  if (c->vertex_buffer.buffer && c->index_buffer.buffer)
  {
    flush();

    ID3D11Buffer *const_buffers[] = { s->const_buffer, c->const_buffer };

    const UINT stride = sizeof(dx11_chunk_vertex_t), offset = 0;

    dx11_shader_bind(s->chunk_shader);
    ID3D11DeviceContext_IASetVertexBuffers(s->device_context, 0, 1, &c->vertex_buffer.buffer,
      &stride, &offset);
    ID3D11DeviceContext_IASetIndexBuffer(s->device_context, c->index_buffer.buffer,
      DXGI_FORMAT_R32_UINT, 0);
    ID3D11DeviceContext_IASetPrimitiveTopology(s->device_context,
      D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetConstantBuffers(s->device_context, 0, 2, const_buffers);
    ID3D11DeviceContext_PSSetShaderResources(s->device_context, 1, 1,
        (ID3D11ShaderResourceView**)&s->tiles_texture.data);
    ID3D11DeviceContext_PSSetSamplers(s->device_context, 0, 1, &s->sampler_state);
    ID3D11DeviceContext_DrawIndexed(s->device_context, c->num_indices, 0, 0);
  }
}
