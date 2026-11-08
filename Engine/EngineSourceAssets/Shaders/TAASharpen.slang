Texture2D<float4> g_input : register(t0);

cbuffer TemporalSharpenParams : register(b0)
{
    float2 g_texelSize;
    float g_sharpness;
    float g_pad;
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

[shader("fragment")]
float4 PSMain(VSOut input) : SV_Target
{
    uint width;
    uint height;
    g_input.GetDimensions(width, height);
    const int2 maxPixel = int2(width, height) - 1;
    const int2 pixel = int2(input.pos.xy);

    const float3 c = g_input.Load(int3(pixel, 0)).rgb;
    const float3 n = g_input.Load(int3(clamp(pixel + int2(0, -1), int2(0, 0), maxPixel), 0)).rgb;
    const float3 s = g_input.Load(int3(clamp(pixel + int2(0, 1), int2(0, 0), maxPixel), 0)).rgb;
    const float3 e = g_input.Load(int3(clamp(pixel + int2(1, 0), int2(0, 0), maxPixel), 0)).rgb;
    const float3 w = g_input.Load(int3(clamp(pixel + int2(-1, 0), int2(0, 0), maxPixel), 0)).rgb;

    const float3 minC = min(c, min(min(n, s), min(e, w)));
    const float3 maxC = max(c, max(max(n, s), max(e, w)));
    const float3 sharpened = c + g_sharpness * (4.0f * c - n - s - e - w) * 0.25f;
    return float4(clamp(sharpened, minC, maxC), 1.0f);
}
