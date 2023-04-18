struct VSInput{
    float3 pos : POSITION;
    float2 tex : TEXCOORD0;
};

struct PSInput{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

SamplerState samp : register(s0);
Texture2D<float4> tex : register(t0);


PSInput VSMain(VSInput input){
    PSInput output;
    output.pos = float4(input.pos, 1.0f);
    output.tex = input.tex;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET{
    return tex.Sample(samp, input.tex);
}