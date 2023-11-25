#include "common.hlsli"

struct vs_input_t
{
  float2 pos   : INPUT0;
  float4 color : INPUT1;
  float2 uv    : INPUT2;
};

struct vs_output_t
{
  float4 pos   : SV_POSITION;
  float4 color : COLOR0;
  float2 uv    : TEXCOORD0;
};

Texture2D<float4> the_texture : register(t0);
SamplerState the_sampler : register(s0);

vs_output_t vs_main(vs_input_t input)
{
  vs_output_t output = (vs_output_t)0;
  output.pos = float4(pixel_to_ndc(input.pos), 0.0, 1.0);
  output.color = input.color;
  output.uv = input.uv;
  return output;
}

float4 ps_main(vs_output_t input) : SV_TARGET
{
  return the_texture.Sample(the_sampler, input.uv) * input.color;
}
