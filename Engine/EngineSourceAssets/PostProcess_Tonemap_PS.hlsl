Texture2D inputTex : register(t0);
SamplerState linearSampler : register(s0);

cbuffer TonemapParams : register(b0)
{
    float g_exposure;
}

struct PSIn
{
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD0;
};

float3 PBRNeutralTonemap(float3 color)
{
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;

    const float d = 1.0 - startCompression;
    float newPeak = 1.0 - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
    return lerp(color, float3(newPeak, newPeak, newPeak), g);
}

float4 main(PSIn input) : SV_Target
{
    float3 c = inputTex.Sample(linearSampler, input.texCoord).rgb;
    c = max(c * g_exposure, 0.0);
    return float4(PBRNeutralTonemap(c), 1.0);
}
