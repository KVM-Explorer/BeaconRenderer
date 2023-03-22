struct VSInput {
  float3 Position : POSITION;
  float3 Normal : NORMAL;
};

struct PSInput {
  float4 Position : SV_POSITION;
  float3 Normal : NORMAL;
  float3 WorldPos : TEXCOORD;
};

struct PSOutput {
  float3 Albedo : SV_TARGET0;
  float4 Normal : SV_TARGET1;
  float4 Specgloss : SV_TARGET2;
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

PSInput VSMain(VSInput input) {
  PSInput res;
  float4 pos = float4(input.Position, 1);
  res.Position = mul(pos, gViewProject);
  res.Normal = input.Normal;
  res.WorldPos = res.Position / res.Position.w; // TODO Understand

  return res;
}

PSOutput PSMain(PSInput input) : SV_TARGET {
  PSOutput output;
  output.Albedo = float3(1.0f, 0.0f, 0.0f);
  output.Normal = float4(input.Normal, 1.0f);

  output.Specgloss = float4(0.5, 0.5, 0.5, 0.6);
  return output;
}
