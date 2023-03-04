struct VSInput {
  float3 Position : POSITION;
  float2 UV : TEXCOORD;
  float3 Normal : NORMAL;
};

struct PSInput {
  float4 Position : SV_POSITION;
  float2 UV : TEXCOORD;
  float3 NORMAL : NORMAL;
};

cbuffer EntityInfo : register(b0) {
  float4x4 Transform;
  uint MaterialIndex;
  uint Padding0;
  uint Padding1;
  uint Padding2;
};

PSInput VSMain(VSInput vin) {
  PSInput res;

  return res;
}

float4 PSMain(PSInput pin) {
  float4 color;

  return color;
}