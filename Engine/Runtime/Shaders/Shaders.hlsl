struct PSInput
{
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    
    // This can be ommited in pixel shader, but the memory structure must match, so we place it last.
    float4 position : SV_POSITION;
};

struct ModelViewProjection
{
    matrix MVP;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

cbuffer TransformCB : register(b1)
{
    matrix modelMatrix;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR, float2 uv: TEXCOORD)
{
    PSInput result;

    //result.position = position;
    position = mul(modelMatrix, position);
    result.position = mul(ModelViewProjectionCB.MVP, position);
    result.color = color;
    result.uv = uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color * float4(input.uv, 1.0, 1.0);
    
    
    //return float4(input.uv, 0, 1); // Debugging (show UVs as colors)
    float2 uv;
    uv.x = input.uv.x;
    uv.y = 1 - input.uv.y;  // Flip UVs (DirectX vs OpenGL)
    float4 textureColor = g_texture.Sample(g_sampler, uv);
    // textureColor.a = 1.0f;
    // return textureColor * float4(input.uv, 0.0f, 1.0f);
    return g_texture.Sample(g_sampler, uv) * input.color;
}