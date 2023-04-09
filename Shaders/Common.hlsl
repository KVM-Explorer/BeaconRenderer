cbuffer SceneInfo : register(b1) {
  float4x4 gView;
  float4x4 gInvView;
  float4x4 gProject;
  float4x4 gInvProject;
  float4x4 gViewProject;
  float4x4 gInvViewProject;
  float4x4 gInvScreenModel;
  float3 gEyePosition;
  float gPadding0;
  float4 gAmbient; // 环境光
}

struct VSInput {
  float3 Position : POSITION;
  float3 Normal : NORMAL;
  float2 Texcoord : TEXCOORD;
};

cbuffer EntityInfo : register(b0) {
  float4x4 gEntityTransform;
  uint gMaterialID;
  uint gShaderID;
  uint Padding1;
  uint Padding2;
};

uint gQuadType : register(b0, space1);

SamplerState gSamplerPointWrap        : register(s0);
SamplerState gSamplerPointClamp       : register(s1);
SamplerState gSamplerLinearWrap       : register(s2);
SamplerState gSamplerLinearClamp      : register(s3);
SamplerState gSamplerAnisotropicWrap  : register(s4);
SamplerState gSamplerAnisotropicClamp : register(s5);
