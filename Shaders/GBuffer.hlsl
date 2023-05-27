#include "Common.hlsl"
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



PSInput VSMain(VSInput input) {
	PSInput res;
	float4 pos = float4(input.Position, 1);
	pos = mul(pos,gEntityTransform);
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
