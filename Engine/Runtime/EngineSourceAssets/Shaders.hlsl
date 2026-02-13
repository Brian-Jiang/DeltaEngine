struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    
    //float4x4 worldMatrix : INSTANCE_WORLD;
    //float3 instanceColor : INSTANCE_COLOR;
};

struct PSInput
{
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 worldPosition : TEXCOORD1;
    float4 position : SV_POSITION;
};

//struct ModelViewProjection
//{
//    matrix MVP;
//};

//ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

struct Camera
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4 position;
};

ConstantBuffer<Camera> CameraCB : register(b0);

struct Object
{
    float4x4 worldMatrix;
    float4 color;
    uint useInstanceMatrix; // 1 = use INSTANCE_WORLD from vertex buffer, 0 = use worldMatrix above
};

ConstantBuffer<Object> ObjectCB : register(b1);

struct Light
{
    float4 position;
    float4 color;
    float intensity;
};

ConstantBuffer<Light> LightCB : register(b2);

Texture2D g_texture : register(t0);
//Texture2D g_texture1 : register(t1);
SamplerState g_sampler : register(s0);

PSInput VSMain(VSInput input)
{
    PSInput result;

    float4 position = float4(input.position, 1.0f);
    float4 worldPosition;
    //if (ObjectCB.useInstanceMatrix != 0)
    //    worldPosition = mul(input.worldMatrix, position);
    //else
    //    worldPosition = mul(ObjectCB.worldMatrix, position);
    
    worldPosition = mul(ObjectCB.worldMatrix, position);
    float4 viewPosition = mul(CameraCB.viewMatrix, worldPosition);
    float4 cameraPosition = mul(CameraCB.projectionMatrix, viewPosition);
    
    result.position = cameraPosition;
    result.worldPosition = worldPosition.xyz;
    //result.color = input.color * float4(input.instanceColor, 1.0);
    result.color = input.color * ObjectCB.color;
    result.normal = input.normal;
    result.uv = input.uv;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    //return float4(CameraCB.position);
    
    
    float4 textureColor = g_texture.Sample(g_sampler, input.uv);
    textureColor = float4(1.0, 0.0, 1.0, 1.0);
    
    float3 left = normalize(LightCB.position.xyz - input.worldPosition);
    float3 right = reflect(left, input.normal);
    float3 view = normalize(CameraCB.position.xyz - input.worldPosition);
    float lightDistance = length(LightCB.position.xyz - input.worldPosition);
    float falloff = 1.0f / (lightDistance * lightDistance);
    //falloff = 1.0f;
    //return float4(left, 1.0f);
    
    float4 lightColor = LightCB.color;
    float4 ambient = float4(0.1f, 0.1f, 0.1f, 1.0f) * input.color * textureColor;
    float4 diffuse = max(dot(input.normal, left), 0.0) * LightCB.intensity * falloff * input.color * textureColor * lightColor;
    float4 specular = pow(max(dot(right, view), 0.0), 32) * LightCB.intensity * falloff * lightColor;
    
    float4 color = ambient + diffuse + specular;
    return color;
    
    
    //return float4(input.uv, 0, 1); // Debugging (show UVs as colors)
    //float2 uv;
    //uv.x = input.uv.x;
    ////uv.x = (input.uv.x + 1.0) / 2.0;
    ////uv.y = 1 - input.uv.y;  // Flip UVs (DirectX vs OpenGL)
    //uv.y = input.uv.y;
    ////uv.y = (input.uv.y + 1.0) / 2.0;
    
    
    //if (textureColor.a < 0.1f)
    //{
    //    discard;
    //}
    
    // textureColor.a = 1.0f;
    // return textureColor * float4(input.uv, 0.0f, 1.0f);
    //return textureColor;
    //return float4(textureColor.rgb, textureColor.a);
}