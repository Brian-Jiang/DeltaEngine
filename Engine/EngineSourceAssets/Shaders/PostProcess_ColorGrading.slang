Texture2D inputTex : register(t0);
SamplerState linearSampler : register(s0);

cbuffer ColorGradingParams : register(b0)
{
    float g_saturation;
    float g_contrast;
    float4 g_lift;
    float4 g_gamma;
    float4 g_gain;
};

struct VSOut
{
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD0;
};

[shader("vertex")]
VSOut VSMain(uint id : SV_VertexID)
{
    float2 uv = float2((id << 1) & 2, id & 2);
    VSOut o;
    o.pos = float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
    o.texCoord = float2(uv.x, 1.0f - uv.y);
    return o;
}

float3 ApplyLiftGammaGain(float3 color)
{
    float3 lifted = color * g_gain.rgb + g_lift.rgb;
    float3 safeGamma = max(g_gamma.rgb, 1e-4);
    return pow(max(lifted, 0.0), 1.0 / safeGamma);
}

float3 ApplyContrast(float3 color, float contrast)
{
    const float pivot = 0.18;
    return (color - pivot) * contrast + pivot;
}

float3 ApplySaturation(float3 color, float saturation)
{
    const float luma = dot(color, float3(0.2126, 0.7152, 0.0722));
    return lerp(float3(luma, luma, luma), color, saturation);
}

[shader("fragment")]
float4 PSMain(VSOut input) : SV_Target
{
    float3 c = inputTex.Sample(linearSampler, input.texCoord).rgb;
    c = ApplyLiftGammaGain(c);
    c = ApplyContrast(c, g_contrast);
    c = ApplySaturation(c, g_saturation);
    return float4(max(c, 0.0), 1.0);
}
