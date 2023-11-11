#include "common.hlsli"

struct sprite_t
{
  int2  src_pos, src_size;
  int2  dst_pos, dst_size;
  float rotation;
};

struct pixel_t
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
};

cbuffer sprite_data : register(b1)
{
  float2 rc_atlas_size;
};

StructuredBuffer<sprite_t> sprite_buffer : register(t0);
Texture2D<float4> atlas_texture : register(t1);
SamplerState the_sampler : register(s0);

pixel_t vs_main(uint sprite_id : SV_INSTANCEID, uint vertex_id : SV_VERTEXID)
{
  sprite_t sprite = sprite_buffer[sprite_id];

  float s = sin(sprite.rotation);
  float c = cos(sprite.rotation);
  uint2 i = { (vertex_id & 2) >> 1, 1 - (vertex_id & 1) };
  float2 xi = { sprite.dst_size.x * c, sprite.dst_size.y * s };
  float2 yi = { -sprite.dst_size.x * s, sprite.dst_size.y * c };
  float2 dst = float2(sprite.dst_pos.x + xi.x * i.x + xi.y * i.y,
    sprite.dst_pos.y + yi.x * i.x + yi.y * i.y);

  float4 src = float4(sprite.src_pos, sprite.src_pos + sprite.src_size);
  uint2 si = { vertex_id & 2, (vertex_id << 1 & 2) ^ 3 };

  pixel_t pixel;
  pixel.pos = float4(pixel_to_ndc(dst), 0.0, 1.0);
  pixel.uv = float2(src[si.x], src[si.y]) * rc_atlas_size;

  return pixel;
}

float4 ps_main(pixel_t pixel) : SV_TARGET
{
  float4 color = atlas_texture.Sample(the_sampler, pixel.uv);
  return color;
}
