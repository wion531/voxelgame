cbuffer const_data : register(b0)
{
  float2 u_rc_window_size;
  float4x4 u_projection;
  float4x4 u_view;
  float4x4 u_model;
};

float2 pixel_to_ndc(float2 pixel)
{
  return ((pixel * u_rc_window_size * 2.0) - 1.0) * float2(1.0, -1.0);
}
