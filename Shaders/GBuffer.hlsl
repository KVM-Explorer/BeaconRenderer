#include "Common.hlsl"
struct VSInput {
	float3 Position : POSITION;
	float3 Normal : NORMAL;
};

struct PSInput {
	float4 Position : SV_POSITION;
	float3 Normal : NORMAL;
	float3 WorldPos : POSITION0;
	float3 OriginPos : POSITION1;
};

struct PSOutput {
	float3 Albedo : SV_TARGET0;
	float4 Normal : SV_TARGET1;
	float4 Specgloss : SV_TARGET2;
};


// cbuffer EntityInfo : register(b0) {
//   float4x4 gEntityTransform;
//   uint gMaterialIndex;
//   uint Padding0;
//   uint Padding1;
//   uint Padding2;
// };


PSInput VSMain(VSInput input) {
	PSInput res;
	float4 pos = float4(input.Position, 1);
	res.Position = mul(pos, gViewProject);
	res.Normal = input.Normal;
	res.WorldPos = input.Position ; // TODO Understand
	res.OriginPos = res.Position.xyz;
	return res;
}

PSOutput PSMain(PSInput input) : SV_TARGET {
	PSOutput output;

	float4 invPos = mul(input.Position,gInvViewProject);
	float3 normPos = (invPos / invPos.w).xyz;
	output.Albedo = normalize(normPos);
	output.Normal = float4(input.Normal, 1.0f);
	output.Specgloss = normalize(invPos);
	
	// output.MaterialID = 1;
	return output;
}
