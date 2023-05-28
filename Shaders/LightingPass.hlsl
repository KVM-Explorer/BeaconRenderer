#include "Common.hlsl"
#include "LightHelper.hlsl"

struct PSInput {
  float4 Position : SV_POSITION;
  float2 Texcoord : TEXCOORD;
};

struct MaterialInfo {
  float4 BaseColor;
  float3 FresnelR0;
  float Roughness;
  uint DiffuseMap;
};

StructuredBuffer<Light> PointLights : register(t0, space1);
StructuredBuffer<MaterialInfo> Materials : register(t1, space1);
Texture2D<float4> gDiffuseMap[100] : register(t0, space2);
TextureCube gCubeMap : register(t0, space3);
Texture2D<float4> gNormalTexture : register(t0);
Texture2D<float2> gUVTexture : register(t1);
Texture2D<uint> gMaterialTexture : register(t2);
Texture2D<uint> gShaderTexure
    : register(t3); // Skybox == 0, Opaque == 1 , Shadow == 2
Texture2D gDepth : register(t4);

PSInput VSMain(VSInput input) {
  PSInput output;
  output.Position = float4(input.Position, 1.0);
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
  float3 normalW = normalize( mul(normal, (float3x3)gViewProject));
  float3 litColor = float3(0, 0, 0);

  Material mat;
  mat.Diffuse = Materials[materialID].BaseColor;
  mat.FresnelR0 = Materials[materialID].FresnelR0;
  mat.Roughness = Materials[materialID].Roughness;
  uint diffuseMap = Materials[materialID].DiffuseMap;

  if (gShaderTexure[input.Position.xy] == 1) { // Opaque
    float4 ambient = gAmbient * mat.Diffuse;

    litColor = PointLightColor(PointLights[0], mat, worldPos.xyz, normal,
                            gEyePosition);
    litColor += ambient.rgb;                // 反射光 + 漫反射光

    float shininess = 1 - mat.Roughness;
    float3 r = reflect(-gEyePosition, worldPos.xyz);
    float4 reflectionColor = gCubeMap.Sample(gSamplerLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, normalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;

    return float4(litColor, mat.Diffuse.a); // 材质提取alpha
  }
  if (gShaderTexure[input.Position.xy] == 2) { // Opaque
    mat.Diffuse = gDiffuseMap[diffuseMap].Sample(gSamplerLinearClamp, uv);
    float4 ambient = gAmbient * mat.Diffuse;
    litColor = PointLightColor(PointLights[0], mat, worldPos.xyz, normal,
                            gEyePosition);
    litColor += ambient.rgb;                // 反射光 + 漫反射光
    return float4(litColor, mat.Diffuse.a); // 材质提取alpha
  }

  return float4(0.14901, 0.14901, 0.14901, 1.0F);
}