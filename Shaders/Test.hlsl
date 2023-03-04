struct VSInput{
    float3 Position: POSITION;
    float4 Color: COLOR;
};

struct PSInput{
    float4 Position : SV_POSITION;
    float4 Color :COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput res;
    res.Position = float4(input.Position,1);
    res.Color = input.Color;
    return res;
}

float4 PSMain(PSInput input) :SV_TARGET
{
    return input.Color;
}

