struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float4 color : COLOR, float2 uv: TEXCOORD)
{
    PSInput result;

    result.position = position;
    result.color = color;
    result.uv = uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    //return float4(input.uv, 0, 1); // Debugging (show UVs as colors)
    float2 uv;
    uv.x = input.uv.x;
    uv.y = 1 - input.uv.y;  // Flip UVs (DirectX vs OpenGL)
    float4 textureColor = g_texture.Sample(g_sampler, uv);
    // textureColor.a = 1.0f;
    // return textureColor * float4(input.uv, 0.0f, 1.0f);
    return g_texture.Sample(g_sampler, uv) * input.color;
}