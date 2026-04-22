Texture2D inputTex : register(t0);
SamplerState linearSampler : register(s0);

struct PSIn
{
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD0;
};

float4 main(PSIn input) : SV_Target
{
    return inputTex.Sample(linearSampler, input.texCoord);
}
