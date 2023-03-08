struct VSInput {
  float3 Position : POSITION;
  float2 UV : TEXCOORD;
  float3 Normal : NORMAL;
};

struct PSInput {
  float4 Position : SV_POSITION;
  float2 UV : TEXCOORD;
  float3 Normal : NORMAL;
};

cbuffer SceneInfo : register(b1) {
  float4x4 gView;
  float4x4 gInvView;
  float4x4 gProject;
  float4x4 gInvProject;
  float4x4 gViewProject;
  float4x4 gInvViewProject;
  float3 gEyePosition;
}

cbuffer EntityInfo : register(b0) {
  float4x4 gEntityTransform;
  uint gMaterialIndex;
  uint Padding0;
  uint Padding1;
  uint Padding2;
};

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

Texture2D gDiffuseMap[20] : register(t0, space1);

PSInput VSMain(VSInput vin) {
  PSInput res;

  float4 pos = mul(float4(vin.Position, 1.F), gEntityTransform);
  res.Position = mul(pos, gViewProject);
  res.UV = vin.UV;
  res.Normal = vin.Normal;
  return res;
}

float4 PSMain(PSInput pin) : SV_TARGET {

  float4 color = gDiffuseMap[gMaterialIndex].Sample(gsamLinearClamp,pin.UV);
  return color;
}