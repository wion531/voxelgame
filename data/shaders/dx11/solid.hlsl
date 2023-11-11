#include "common.hlsli"

struct vs_input_t
{
  float2 pos : POSITION;
  float4 color : COLOR;
};

struct vs_output_t
{
  float4 pos : SV_POSITION;
  float4 color : COLOR0;
};

vs_output_t vs_main(vs_input_t input)
{
  vs_output_t output = (vs_output_t)0;
  output.pos = float4(pixel_to_ndc(input.pos), 0.0, 1.0);
  output.color = input.color;
  return output;
}

float4 ps_main(vs_output_t input) : SV_TARGET
{
  return input.color;
}
