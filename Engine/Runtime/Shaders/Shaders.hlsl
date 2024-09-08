struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
};

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
//Texture2D g_texture1 : register(t1);
SamplerState g_sampler : register(s0);

cbuffer TransformCB : register(b1)
{
    matrix modelMatrix;
};

PSInput VSMain(VSInput input)
{
    PSInput result;

    //result.position = position;
    //position = mul(modelMatrix, position);
    result.position = mul(ModelViewProjectionCB.MVP, float4(input.position, 1.0f));
    result.color = input.color;
    //result.color = float4(position.z / 10.0f, 0.0f, 0.0f, 1.0f);
    result.uv = input.uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    //return float4(input.position.z / 2.0f, 0.0f, 0.0f, 1.0f);
    //return input.color * float4(input.uv, 1.0, 1.0);
    //return input.color;
    
    
    //return float4(input.uv, 0, 1); // Debugging (show UVs as colors)
    float2 uv;
    uv.x = input.uv.x;
    //uv.x = (input.uv.x + 1.0) / 2.0;
    //uv.y = 1 - input.uv.y;  // Flip UVs (DirectX vs OpenGL)
    uv.y = input.uv.y;
    //uv.y = (input.uv.y + 1.0) / 2.0;
    float4 textureColor = g_texture.Sample(g_sampler, uv);
    
    if (textureColor.a < 0.1f)
    {
        discard;
    }
    
    // textureColor.a = 1.0f;
    // return textureColor * float4(input.uv, 0.0f, 1.0f);
    return textureColor;
    //return float4(textureColor.rgb, textureColor.a);
}