#include "Common.hlsl"
Texture2D gTexture1 : register(t0); 
Texture2D gTexture2 : register(t1);

struct PSInput {
  float4 Position : SV_POSITION;
  float2 Texcoord : TEXCOORD;
};


PSInput VSMain(VSInput input) {
  PSInput output;
  output.Position = float4(input.Position,1.0F);
  output.Texcoord = input.Texcoord;
  return output;
}

float4 PSMain(PSInput input) : SV_TARGET { // TODO 匹配DataStruct中的Enum
  float4 color;
  float4 texColor1;
  float4 texColor2;
  switch (gQuadType) {
  case 0:
    color = float4(1.0, 0.0, 0.0, 1.0);
    break;
  case 1:
    color = gTexture1.Sample(gSamplerLinearClamp, input.Texcoord);
    break;
  case 2:
    texColor1 = gTexture1.Sample(gSamplerLinearClamp, input.Texcoord);
    texColor2= gTexture2.Sample(gSamplerLinearClamp, input.Texcoord);
    color = texColor1 * texColor2;
    break;
  default:
    color = float4(0.0, 1.0, 0.0, 1.0);
    break;
  }
  return color;
}