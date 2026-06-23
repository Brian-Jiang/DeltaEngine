Texture2D g_inputTex : register(t0);
Texture2D g_sceneTex : register(t0);
Texture2D g_bloomTex : register(t1);
SamplerState g_linearSampler : register(s0);

cbuffer BloomParams : register(b0)
{
    float g_threshold;
    float g_softKnee;
    float g_intensity;
    float g_texelSize;
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

float Luminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

[shader("fragment")]
float4 PSExtract(VSOut input) : SV_Target
{
    float3 color = g_inputTex.Sample(g_linearSampler, input.texCoord).rgb;
    float lum = Luminance(color);

    float knee = g_threshold * g_softKnee;
    float soft = lum - g_threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = (soft * soft) / (4.0 * knee + 1e-4);

    float contribution = max(soft, lum - g_threshold);
    contribution /= max(lum, 1e-4);

    return float4(color * contribution, 1.0);
}

[shader("fragment")]
float4 PSBlur(VSOut input) : SV_Target
{
    float2 uv = input.texCoord;
    float2 offset = float2(g_texelSize, g_texelSize);

    float4 sum = g_inputTex.Sample(g_linearSampler, uv) * 4.0;
    sum += g_inputTex.Sample(g_linearSampler, uv - offset);
    sum += g_inputTex.Sample(g_linearSampler, uv + offset);
    sum += g_inputTex.Sample(g_linearSampler, uv + float2(offset.x, -offset.y));
    sum += g_inputTex.Sample(g_linearSampler, uv - float2(offset.x, -offset.y));
    return sum / 8.0;
}

[shader("fragment")]
float4 PSComposite(VSOut input) : SV_Target
{
    float3 scene = g_sceneTex.Sample(g_linearSampler, input.texCoord).rgb;
    float3 bloom = g_bloomTex.Sample(g_linearSampler, input.texCoord).rgb;
    return float4(scene + bloom * g_intensity, 1.0);
}
