#include "Common.hlsl"
Texture2D gTexture1 : register(t0);
Texture2D gTexture2 : register(t1);

struct PSInput {
  float4 Position : SV_POSITION;
  float2 Texcoord : TEXCOORD;
};

uint gQuadID : register(b0, space1); // 无法绑定

PSInput VSMain(VSInput input) {
  PSInput output;
  output.Position = float4(input.Position, 1.0F);
  output.Texcoord = input.Texcoord;
  return output;
}

float4 PSMain(PSInput input) : SV_TARGET { // TODO 匹配DataStruct中的Enum
  float4 color;
  float4 texColor1;
  float4 texColor2;
  // return float4(0.3,0.5,0.2,1);
  texColor1 = gTexture1.Sample(gSamplerLinearClamp, input.Texcoord);
  texColor2 = gTexture2.Sample(gSamplerLinearClamp, input.Texcoord);
  color = texColor1 * texColor2;

  return color;
}