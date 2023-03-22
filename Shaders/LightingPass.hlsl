
struct VSInput {
  float4 Position : POSITION;
  float2 Texcoord : TEXCOORD;
};

struct PSInput {
  float4 Position : SV_POSITION;
  float2 Texcoord : TEXCOORD;
};

Texture2D gAlbedoTexture : register(t0);
Texture2D gNormalTexture : register(t1);
Texture2D gSpecularGlossTexture : register(t2);
Texture2D gDepth : register(t3);

sampler gLinearSample;

PSInput VSMain(VSInput input) {
  PSInput output;
  output.Position = input.Position;
  output.Texcoord = input.Texcoord;
  return output;
}

float4 PSMain(PSInput input) : SV_TARGET{
  
  float4 color = gAlbedoTexture.Sample(gLinearSample,input.Texcoord);
  return color;
}