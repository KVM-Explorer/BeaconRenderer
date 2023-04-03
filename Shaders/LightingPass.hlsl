#include "Common.hlsl"
#include "LightHelper.hlsl"

struct VSInput {
  float4 Position : POSITION;
  float2 Texcoord : TEXCOORD;
};

struct PSInput {
  float4 Position : SV_POSITION;
  float2 Texcoord : TEXCOORD;
};

StructuredBuffer<Light> PointLights : register(t0, space1);

Texture2D<float4> gNormalTexture : register(t0);
Texture2D<float2> gUVTexture : register(t1);
Texture2D<uint> gMaterialTexture : register(t2);
Texture2D<uint> gShaderID : register(t3);
Texture2D gDepth : register(t4);

PSInput VSMain(VSInput input) {
  PSInput output;
  output.Position = input.Position;
  output.Texcoord = input.Texcoord;
  return output;
}

float4 PSMain(PSInput input) : SV_TARGET {

  float z = gDepth[input.Position.xy].x;
  float4 projectPos = float4(input.Position.xy, z, 1.0F);
  float4 worldPos = mul(projectPos, gInvScreenModel);

  worldPos = mul(worldPos, gInvViewProject);
  worldPos = worldPos / worldPos.w;

  float2 uv = gUVTexture[input.Position.xy].xy;
  uint materialID = gMaterialTexture[input.Position.xy].x;

  float3 tmp = gNormalTexture[input.Position.xy].xyz;
  float3 normal = normalize(tmp);
  float3 color = float3(0, 0, 0);
  Material mat;
  mat.Diffuse = float4(0.5, 0.64, 1.0, 1.0);
  mat.Roughness = 0.2;
  // mat.FresnelR0 = float3(0.08, 0.08, 0.08);
  mat.FresnelR0 = uv.xyx;

  if (gShaderID[input.Position.xy] == 1) {
    float4 ambient = gAmbient * mat.Diffuse;
    color = PointLightColor(PointLights[0], mat, worldPos.xyz, normal,
                            gEyePosition);
    color += ambient.rgb;                    // 反射光 + 漫反射光
    return float4(color, mat.Diffuse.a); // 材质提取alpha
  }
  return float4(0, 0, 0, 1.0F);
}