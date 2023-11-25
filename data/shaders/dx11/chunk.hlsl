#include "common.hlsli"

/*
struct vs_input_t
{
  float3 pos : POSITION;
  float2 uv : TEXCOORD;
  float  brightness : COLOR;
};
*/
struct vs_input_t
{
  uint data : INPUT;
};

struct vs_output_t
{
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float4 color : COLOR0;
};

cbuffer chunk_uniform : register(b1)
{
  float2 u_position;
  float2 u_rc_atlas_size;
};

Texture2D<float4> atlas_texture : register(t1);
SamplerState the_sampler : register(s0);

vs_output_t vs_main(vs_input_t input)
{
  vs_output_t output = (vs_output_t)0;

  uint x        = (input.data & 0xf8000000) >> (32 - 5);
  uint z        = (input.data & 0x07c00000) >> (32 - 10);
  uint y        = (input.data & 0x003fc000) >> (32 - 18);
  uint block    = (input.data & 0x00003fc0) >> (32 - 26);
  uint texcoord = (input.data & 0x00000030) >> (32 - 28);
  uint light    = (input.data & 0x0000000f) >> (32 - 32);

  float2 k_tile_size = float2(16.0, 16.0) * u_rc_atlas_size;
  float2 uvs[] = {
    { 0, 0 },
    { 1, 0 },
    { 0, 1 },
    { 1, 1 },
  };
  float2 tile_pos = float2(block % 16, block / 16);
  float brightness = 0.5 + ((float)light * (0.5 / 16));

  float4 chunk_pos = float4(u_position.x, 0.0, u_position.y, 1.0);
  output.pos = mul(mul(u_projection, u_view), (float4(x, y, z, 1.0) + chunk_pos));
  output.uv = (tile_pos + uvs[texcoord]) * k_tile_size;
  output.color = float4(brightness, brightness, brightness, 1.0);

  // adding a small margin to prevent sampling beyond tile boundaries
  float2 margin = (uvs[texcoord] * 2 - 1) * 0.00001;
  output.uv -= margin;

  return output;
}

float4 ps_main(vs_output_t input) : SV_TARGET
{
  float tile_size = 16.0f * u_rc_atlas_size.x;
  float4 res = atlas_texture.Sample(the_sampler, input.uv);
  if (res.a < 0.1)
    discard;
  return res * input.color;
};
