#define COBJMACROS
#include "../constants.h"
#include "../game.h"
#include "../memory.h"
#include "../system.h"
#include "../gpu.h"
#include <d3d11.h>
#include <dxgi.h>
#include <string.h>
#include <stdio.h>

#define CHECK(expr) do {\
    HRESULT hr_ = expr;\
    WT_UNUSED(hr_);\
    if (FAILED(hr_))\
      printf("error code: %lx\n", hr_);\
    WT_ASSERT(SUCCEEDED(hr_) && #expr " failed");\
  } while (0)

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

  wt_pool_t buffer_pool;
  wt_pool_t shader_pool;
  wt_pool_t texture_pool;

  wt_vec2_t window_size;
} gpu_state_t;

typedef struct
{
  ID3D11Buffer *buffer;
  ID3D11ShaderResourceView *srv;
  gpu_buffer_type_t type;
  usize stride;
  gpu_index_type_t index_type;
  gpu_shader_stage_t shader_stage;
} dx11_buffer_t;

typedef struct
{
  ID3D11VertexShader *vs;
  ID3D11PixelShader *ps;
  ID3D11InputLayout *input_layout;
} dx11_shader_t;

typedef struct
{
  ID3D11Texture2D *texture;
  ID3D11ShaderResourceView *srv;
  int width, height;
} dx11_texture_t;

static gpu_state_t *get_state(void)
{
  return game_get_state()->modules.gpu;
}

static void safe_release(void *obj)
{
  if (obj)
  {
    ID3D11Resource *rsrc = (ID3D11Resource*)obj;
    rsrc->lpVtbl->Release(rsrc);
  }
}

void gpu_init(void)
{
  game_state_t *gs = game_get_state();
  gpu_state_t *s = gs->modules.gpu = mem_hunk_push(sizeof(gpu_state_t));

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

  // === pools ===
  void *pool_buffer = mem_hunk_push(wt_align16(sizeof(dx11_buffer_t)) * GPU_MAX_BUFFERS);
  s->buffer_pool = wt_pool_new(pool_buffer, GPU_MAX_BUFFERS, sizeof(dx11_buffer_t));

  pool_buffer = mem_hunk_push(wt_align16(sizeof(dx11_shader_t)) * GPU_MAX_SHADERS);
  s->shader_pool = wt_pool_new(pool_buffer, GPU_MAX_SHADERS, sizeof(dx11_shader_t));

  pool_buffer = mem_hunk_push(wt_align16(sizeof(dx11_texture_t)) * GPU_MAX_TEXTURES);
  s->texture_pool = wt_pool_new(pool_buffer, GPU_MAX_TEXTURES , sizeof(dx11_texture_t));
}

void gpu_frame_begin(void)
{
  gpu_state_t *s = get_state();
  wt_vec2_t window_size = sys_window_get_size();
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

  D3D11_VIEWPORT viewport = {
    0, 0, window_size.x, window_size.y, 0.0f, 1.0f
  };

  ID3D11DeviceContext_RSSetViewports(s->device_context, 1, &viewport);
  ID3D11DeviceContext_OMSetRenderTargets(s->device_context, 1, &s->render_target_view,
    s->depth_stencil_view);
  ID3D11DeviceContext_OMSetBlendState(s->device_context, s->blend_state, NULL, WT_U32_MAX);
}

void gpu_frame_end(void)
{
  gpu_state_t *s = get_state();
  IDXGISwapChain_Present(s->swap_chain, 1, 0);
}

gpu_buffer_t gpu_buffer_new(gpu_buffer_desc_t *desc)
{
  gpu_state_t *s = get_state();

  if (desc->type == GPU_BUFFER_VERTEX || desc->type == GPU_BUFFER_STRUCTS)
  {
    WT_ASSERT(desc->stride > 0 && "stride must be >0");
  }
  WT_ASSERT(desc->size > 0 && "size must be >0");

  if (desc->type == GPU_BUFFER_STRUCTS)
  {
    WT_ASSERT(desc->shader_stage == GPU_SHADER_STAGE_VERTEX &&
      "struct buffers not supported for pixel shaders");
  }

  dx11_buffer_t *res = wt_pool_alloc(&s->buffer_pool);
  WT_ASSERT(res && "too many buffers");

  res->type = desc->type;
  res->stride = desc->stride;
  res->index_type = desc->index_type;
  res->shader_stage = desc->shader_stage;

  D3D11_BUFFER_DESC bd = { 0 };
  bd.ByteWidth = desc->size;
  bd.Usage = desc->dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
  switch (desc->type)
  {
    case GPU_BUFFER_VERTEX:   bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;   break;
    case GPU_BUFFER_INDEX:    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;    break;
    case GPU_BUFFER_STRUCTS:  bd.BindFlags = D3D11_BIND_SHADER_RESOURCE; break;
    case GPU_BUFFER_CONSTANT:
      bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
      bd.ByteWidth = wt_align16(bd.ByteWidth);
      break;
  }
  bd.CPUAccessFlags = desc->dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
  bd.StructureByteStride = desc->stride;

  D3D11_SUBRESOURCE_DATA data = { 0 };
  data.pSysMem = desc->data;

  CHECK(ID3D11Device_CreateBuffer(s->device, &bd, desc->data ? &data : NULL, &res->buffer));

  if (desc->type == GPU_BUFFER_STRUCTS)
  {
    CHECK(ID3D11Device_CreateShaderResourceView(s->device, (ID3D11Resource*)res->buffer,
      (&(D3D11_SHADER_RESOURCE_VIEW_DESC){
        .Format = DXGI_FORMAT_UNKNOWN,
        .ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
        .Buffer.NumElements = desc->size / desc->stride,
      }), &res->srv));
  }

  return (gpu_buffer_t)res;
}

void gpu_buffer_update(gpu_buffer_t buf, void *data, usize size)
{
  gpu_state_t *s = get_state();
  dx11_buffer_t *db = (dx11_buffer_t*)buf;
  D3D11_MAPPED_SUBRESOURCE mapped = { 0 };

  ID3D11DeviceContext_Map(s->device_context, (ID3D11Resource*)db->buffer, 0,
    D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  WT_ASSERT(mapped.pData);
  memcpy(mapped.pData, data, size);
  ID3D11DeviceContext_Unmap(s->device_context, (ID3D11Resource*)db->buffer, 0);
}

void gpu_buffer_bind(gpu_buffer_t buf, u32 slot)
{
  gpu_state_t *s = get_state();
  dx11_buffer_t *db = (dx11_buffer_t*)buf;

  switch (db->type)
  {
  case GPU_BUFFER_VERTEX:
    u32 offset = 0;
    u32 stride = db->stride;
    ID3D11DeviceContext_IASetVertexBuffers(s->device_context, slot, 1, &db->buffer, &stride, &offset);
    break;
  case GPU_BUFFER_INDEX:
    DXGI_FORMAT index_format = 0;
    switch (db->index_type)
    {
      case GPU_INDEX_INVALID: break;
      case GPU_INDEX_U8:  index_format = DXGI_FORMAT_R8_UINT; break;
      case GPU_INDEX_U16: index_format = DXGI_FORMAT_R16_UINT; break;
      case GPU_INDEX_U32: index_format = DXGI_FORMAT_R32_UINT; break;
      default: break;
    }

    ID3D11DeviceContext_IASetIndexBuffer(s->device_context, db->buffer, index_format, 0);
    break;
  case GPU_BUFFER_STRUCTS:
    WT_ASSERT(db->shader_stage != GPU_SHADER_STAGE_PIXEL && "struct buffers aren't supported for pixel shaders");
    ID3D11DeviceContext_VSSetShaderResources(s->device_context, slot, 1, &db->srv);
  case GPU_BUFFER_CONSTANT:
    if (db->shader_stage == GPU_SHADER_STAGE_VERTEX)
    {
      ID3D11DeviceContext_VSSetConstantBuffers(s->device_context, slot, 1, &db->buffer);
    }
    else
    {
      ID3D11DeviceContext_PSSetConstantBuffers(s->device_context, slot, 1, &db->buffer);
    }
    break;
  }
}

void gpu_buffer_free(gpu_buffer_t buf)
{
  gpu_state_t *s = get_state();
  dx11_buffer_t *db = (dx11_buffer_t*)buf;
  safe_release(db->buffer);
  wt_pool_free(&s->buffer_pool, buf);
}

gpu_texture_t gpu_texture_new(gpu_texture_desc_t *desc)
{
  gpu_state_t *s = get_state();
  dx11_texture_t *res = wt_pool_alloc(&s->texture_pool);
  WT_ASSERT(res && "too many textures");

  res->width = desc->width;
  res->height = desc->height;

  CHECK(ID3D11Device_CreateTexture2D(s->device, (&(D3D11_TEXTURE2D_DESC){
      .Width = res->width,
      .Height = res->height,
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
      .SampleDesc.Count = 1,
      .Usage = (desc->dynamic) ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE,
      .BindFlags = D3D11_BIND_SHADER_RESOURCE,
      .CPUAccessFlags = (desc->dynamic) ? D3D11_CPU_ACCESS_WRITE : 0,
    }), (&(D3D11_SUBRESOURCE_DATA){
      .pSysMem = desc->pixels,
      .SysMemPitch = res->width * 4,
    }), &res->texture));
  CHECK(ID3D11Device_CreateShaderResourceView(s->device, (ID3D11Resource*)res->texture, NULL,
    (ID3D11ShaderResourceView**)&res->srv));

  return (gpu_texture_t)res;
}

wt_vec2_t gpu_texture_get_size(gpu_texture_t tex)
{
  dx11_texture_t *dt = (dx11_texture_t*)tex;
  return wt_vec2(dt->width, dt->height);
}

void gpu_texture_update(gpu_texture_t tex, wt_color_t *data, usize size_bytes)
{
  gpu_state_t *s = get_state();
  dx11_texture_t *dt = (dx11_texture_t*)tex;
  D3D11_MAPPED_SUBRESOURCE mapped = { 0 };

  ID3D11DeviceContext_Map(s->device_context, (ID3D11Resource*)dt->texture, 0,
    D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  WT_ASSERT(mapped.pData);
  memcpy(mapped.pData, data, size_bytes);
  ID3D11DeviceContext_Unmap(s->device_context, (ID3D11Resource*)dt->texture, 0);
}

void gpu_texture_bind(gpu_texture_t tex, u32 slot)
{
  gpu_state_t *s = get_state();
  dx11_texture_t *dt = (dx11_texture_t*)tex;
  ID3D11DeviceContext_PSSetShaderResources(s->device_context, slot, 1, (ID3D11ShaderResourceView**)&dt->srv);
  ID3D11DeviceContext_PSSetSamplers(s->device_context, 0, 1, &s->sampler_state);
}

void gpu_texture_free(gpu_texture_t tex)
{
  gpu_state_t *s = get_state();
  dx11_texture_t *dt = (dx11_texture_t*)tex;
  safe_release(dt->texture);
  safe_release(dt->srv);
  wt_pool_free(&s->texture_pool, dt);
}

static DXGI_FORMAT gpu_data_type_to_dxgi_format(gpu_data_type_t dt)
{
  switch (dt)
  {
    case GPU_DATA_INT:        return DXGI_FORMAT_R32_SINT;
    case GPU_DATA_INT2:       return DXGI_FORMAT_R32G32_SINT;
    case GPU_DATA_INT3:       return DXGI_FORMAT_R32G32B32_SINT;
    case GPU_DATA_INT4:       return DXGI_FORMAT_R32G32B32A32_SINT;
    case GPU_DATA_UINT:       return DXGI_FORMAT_R32_UINT;
    case GPU_DATA_UINT2:      return DXGI_FORMAT_R32G32_UINT;
    case GPU_DATA_UINT3:      return DXGI_FORMAT_R32G32B32_UINT;
    case GPU_DATA_UINT4:      return DXGI_FORMAT_R32G32B32A32_UINT;
    case GPU_DATA_FLOAT:      return DXGI_FORMAT_R32_FLOAT;
    case GPU_DATA_FLOAT2:     return DXGI_FORMAT_R32G32_FLOAT;
    case GPU_DATA_FLOAT3:     return DXGI_FORMAT_R32G32B32_FLOAT;
    case GPU_DATA_FLOAT4:     return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case GPU_DATA_BYTE4_NORM: return DXGI_FORMAT_R8G8B8A8_UNORM;
    default: return 0;
  }
}

gpu_shader_t gpu_shader_new(gpu_shader_desc_t *desc)
{
  mem_scratch_begin();

  WT_ASSERT(desc->vs_path && "must specify vertex shader path");
  WT_ASSERT(desc->ps_path && "must specify pixel shader path");

  gpu_state_t *s = get_state();
  gpu_shader_t res = wt_pool_alloc(&s->shader_pool);
  WT_ASSERT(res && "too many shaders");

  dx11_shader_t *ds = (dx11_shader_t*)res;

  sys_file_contents_t vs_contents = sys_file_read_to_scratch_buffer(desc->vs_path, false);
  sys_file_contents_t ps_contents = sys_file_read_to_scratch_buffer(desc->ps_path, false);

  if (vs_contents.data && ps_contents.data)
  {
    CHECK(ID3D11Device_CreateVertexShader(s->device, vs_contents.data, vs_contents.size, NULL, &ds->vs));
    CHECK(ID3D11Device_CreatePixelShader(s->device, ps_contents.data, ps_contents.size, NULL, &ds->ps));
  }

  D3D11_INPUT_ELEMENT_DESC input_elements[GPU_SHADER_MAX_INPUTS] = { 0 };
  usize num_input_elements = 0;

  for (u64 i = 0; i < GPU_SHADER_MAX_INPUTS; ++i)
  {
    gpu_shader_input_t input = desc->inputs[i];
    if (input.type == GPU_DATA_NULL)
    {
      break;
    }

    input_elements[i].SemanticName = "INPUT";
    input_elements[i].SemanticIndex = i;
    input_elements[i].Format = gpu_data_type_to_dxgi_format(input.type);
    input_elements[i].InputSlot = input.buffer_index;
    input_elements[i].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    input_elements[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    input_elements[i].InstanceDataStepRate = 0;

    num_input_elements += 1;
  }

  CHECK(ID3D11Device_CreateInputLayout(s->device, input_elements, num_input_elements,
    vs_contents.data, vs_contents.size, &ds->input_layout));

  mem_scratch_end();
  return res;
}

void gpu_shader_bind(gpu_shader_t shd)
{
  gpu_state_t *s = get_state();
  dx11_shader_t *ds = (dx11_shader_t*)shd;
  ID3D11DeviceContext_VSSetShader(s->device_context, ds->vs, NULL, 0);
  ID3D11DeviceContext_PSSetShader(s->device_context, ds->ps, NULL, 0);
  ID3D11DeviceContext_IASetInputLayout(s->device_context, ds->input_layout);
}

void gpu_shader_free(gpu_shader_t shd)
{
  gpu_state_t *s = get_state();
  dx11_shader_t *ds = (dx11_shader_t*)shd;
  safe_release(ds->vs);
  safe_release(ds->ps);
  safe_release(ds->input_layout);
  wt_pool_free(&s->shader_pool, shd);
}

gpu_framebuffer_t gpu_framebuffer_new(wt_vec2_t size);
wt_vec2_t gpu_framebuffer_get_size(gpu_framebuffer_t fb);
gpu_texture_t gpu_framebuffer_get_texture(gpu_framebuffer_t fb);
void gpu_framebuffer_bind(gpu_framebuffer_t fb);
void gpu_framebuffer_unbind(gpu_framebuffer_t fb);
void gpu_framebuffer_free(gpu_framebuffer_t fb);

void gpu_clear(wt_color_t clr)
{
  gpu_state_t *s = get_state();
  wt_colorf_t cf = wt_color_to_colorf(clr);
  float color[] = { cf.r, cf.g, cf.b, cf.a };
  ID3D11DeviceContext_ClearRenderTargetView(s->device_context, s->render_target_view, color);
  ID3D11DeviceContext_ClearDepthStencilView(s->device_context, s->depth_stencil_view,
    D3D11_CLEAR_DEPTH, 1.0f, 0);
}

static int gpu_primitive_type_to_primitive_topology(gpu_primitive_type_t pt)
{
  switch (pt)
  {
    case GPU_PRIMITIVE_POINTS:         return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
    case GPU_PRIMITIVE_LINES:          return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
    case GPU_PRIMITIVE_TRIANGLES:      return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case GPU_PRIMITIVE_TRIANGLE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default:                           return 0;
  }
}

void gpu_draw(gpu_primitive_type_t pt, usize offset, usize num_vertices)
{
  gpu_state_t *s = get_state();
  ID3D11DeviceContext_IASetPrimitiveTopology(s->device_context, gpu_primitive_type_to_primitive_topology(pt));
  ID3D11DeviceContext_Draw(s->device_context, num_vertices, offset);
}

void gpu_draw_indexed(gpu_primitive_type_t pt, usize offset, usize num_indices)
{
  gpu_state_t *s = get_state();
  ID3D11DeviceContext_IASetPrimitiveTopology(s->device_context, gpu_primitive_type_to_primitive_topology(pt));
  ID3D11DeviceContext_DrawIndexed(s->device_context, num_indices, offset, 0);
}

void gpu_draw_instanced(gpu_primitive_type_t pt, usize offset, usize num_vertices, usize num_instances)
{
  gpu_state_t *s = get_state();
  ID3D11DeviceContext_IASetPrimitiveTopology(s->device_context, gpu_primitive_type_to_primitive_topology(pt));
  ID3D11DeviceContext_DrawInstanced(s->device_context, num_vertices, num_instances, 0, offset);
}

void gpu_draw_indexed_instanced(gpu_primitive_type_t pt, usize offset, usize num_indices, usize num_instances)
{
  gpu_state_t *s = get_state();
  ID3D11DeviceContext_IASetPrimitiveTopology(s->device_context, gpu_primitive_type_to_primitive_topology(pt));
  ID3D11DeviceContext_DrawIndexedInstanced(s->device_context, num_indices, num_instances, 0, 0, offset);
}
