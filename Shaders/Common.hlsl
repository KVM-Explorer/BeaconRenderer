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