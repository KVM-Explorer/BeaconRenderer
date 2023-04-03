#include "Common.hlsl"
struct VSInput {
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 Texcoord: TEXCOORD;
};

struct PSInput {
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD;
	float3 WorldPos : POSITION;
};

struct PSOutput {
	float4 Normal : SV_TARGET0;
	float2 UV : SV_TARGET1;
  	uint MaterialID : SV_TARGET2;
	uint ShaderID : SV_TARGET3;
};


cbuffer EntityInfo : register(b0) {
  float4x4 gEntityTransform;
  uint gMaterialID;
  uint gShaderID;
  uint Padding1;
  uint Padding2;
};


PSInput VSMain(VSInput input) {
	PSInput res;
	float4 pos = float4(input.Position, 1);
	res.Position = mul(pos, gViewProject);
	res.Normal = input.Normal;
	res.UV = input.Texcoord;
	res.WorldPos = input.Position ; // TODO Understand
	return res;
}

PSOutput PSMain(PSInput input) : SV_TARGET {
	PSOutput output;


	output.Normal = float4(input.Normal, 1.0f);
	output.UV = input.UV;
	output.MaterialID = gMaterialID;
	output.ShaderID = gShaderID;
	return output;
}
