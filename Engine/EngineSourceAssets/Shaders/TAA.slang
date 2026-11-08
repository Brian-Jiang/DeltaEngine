Texture2D<float4> g_current : register(t0);
Texture2D<float4> g_history : register(t1);
Texture2D<float2> g_motion : register(t2);
Texture2D<float> g_depth : register(t3);

SamplerState g_linearClamp : register(s0);
SamplerState g_pointClamp : register(s1);

cbuffer TemporalResolveParams : register(b0)
{
    float2 g_jitterUv;
    float2 g_texelSize;
    float g_blendFactor;
    uint g_historyValid;
}

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

float3 RgbToYCoCg(float3 c)
{
    return float3(
        0.25f * c.r + 0.5f * c.g + 0.25f * c.b,
        0.5f * c.r - 0.5f * c.b,
        -0.25f * c.r + 0.5f * c.g - 0.25f * c.b);
}

float3 YCoCgToRgb(float3 c)
{
    return float3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z);
}

float Luma(float3 c)
{
    return dot(c, float3(0.2126f, 0.7152f, 0.0722f));
}

[shader("fragment")]
float4 PSMain(VSOut input) : SV_Target
{
    const int2 pixel = int2(input.pos.xy);
    const float4 rawCurrent = g_current.Load(int3(pixel, 0));
    if (g_historyValid == 0u)
        return rawCurrent;

    uint width;
    uint height;
    g_current.GetDimensions(width, height);
    const int2 maxPixel = int2(width, height) - 1;

    float3 minYCoCg = float3(1e30f, 1e30f, 1e30f);
    float3 maxYCoCg = float3(-1e30f, -1e30f, -1e30f);
    float closestDepth = 2.0f;
    int2 closestPixel = pixel;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            const int2 p = clamp(pixel + int2(x, y), int2(0, 0), maxPixel);
            const float3 c = RgbToYCoCg(g_current.Load(int3(p, 0)).rgb);
            minYCoCg = min(minYCoCg, c);
            maxYCoCg = max(maxYCoCg, c);

            const float d = g_depth.Load(int3(p, 0));
            if (d < closestDepth)
            {
                closestDepth = d;
                closestPixel = p;
            }
        }
    }

    const float2 uv = (float2(pixel) + 0.5f) * g_texelSize;
    const float3 current = g_current.SampleLevel(g_linearClamp, uv + g_jitterUv, 0.0f).rgb;

    const float2 motion = g_motion.Load(int3(closestPixel, 0));
    const float2 prevUv = uv - motion;
    if (any(prevUv < 0.0f) || any(prevUv > 1.0f))
        return float4(current, 1.0f);

    float3 history = g_history.SampleLevel(g_linearClamp, prevUv, 0.0f).rgb;
    history = YCoCgToRgb(clamp(RgbToYCoCg(history), minYCoCg, maxYCoCg));

    const float currentWeight = g_blendFactor / (1.0f + Luma(current));
    const float historyWeight = (1.0f - g_blendFactor) / (1.0f + Luma(history));
    const float3 resolved = (current * currentWeight + history * historyWeight) / (currentWeight + historyWeight);
    return float4(resolved, 1.0f);
}
