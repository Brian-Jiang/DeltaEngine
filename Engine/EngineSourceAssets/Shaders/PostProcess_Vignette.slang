Texture2D inputTex : register(t0);
SamplerState linearSampler : register(s0);

cbuffer VignetteParams : register(b0)
{
    float g_intensity;
    float g_smoothness;
    float g_roundness;
    float g_padding;
    float4 g_color;
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

[shader("fragment")]
float4 PSMain(VSOut input) : SV_Target
{
    float4 scene = inputTex.Sample(linearSampler, input.texCoord);

    float2 centered = input.texCoord * 2.0f - 1.0f;
    centered.x *= g_roundness;
    float dist = length(centered);

    const float inner = 1.0 - g_smoothness;
    float mask = smoothstep(inner, 1.0, dist);
    mask = saturate(mask * g_intensity);

    float3 result = lerp(scene.rgb, g_color.rgb, mask);
    return float4(result, scene.a);
}
