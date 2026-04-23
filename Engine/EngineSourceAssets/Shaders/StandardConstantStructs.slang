module StandardConstantStructs;

// ==== CBV ====

// Camera (b0)
public struct Camera
{
    public float4x4 viewMatrix;
    public float4x4 projectionMatrix;
    public float4 position;
};

public ConstantBuffer<Camera> CameraCB : register(b0);

// Object (b1)
public struct Object
{
    public float4x4 worldMatrix;
    public float4 color;
    public uint useInstanceMatrix;
};

public ConstantBuffer<Object> ObjectCB : register(b1);

// Light counts (b2)
public struct LightCounts
{
    public uint numDirectionalLights;
    public uint numPointLights;
    public uint numSpotLights;
};

public ConstantBuffer<LightCounts> LightCB : register(b2);

// ==== SRV for Lights ====

public struct PointLight
{
    public float4 position;
    public float4 color;
    public float intensity;
    public float range;
};

public StructuredBuffer<PointLight> PointLights : register(t0);

public struct SpotLight
{
    public float4 position;
    public float4 direction;
    public float4 color;
    public float intensity;
    public float range;
    public float innerConeAngle;
    public float outerConeAngle;
};

public StructuredBuffer<SpotLight> SpotLights : register(t1);

public struct DirectionalLight
{
    public float4 direction;
    public float4 color;
    public float intensity;
};

public StructuredBuffer<DirectionalLight> DirectionalLights : register(t2);

// ==== SRV for Textures ====

public Texture2D g_texture : register(t0, space1);

// ==== Sampler ====

public SamplerState g_sampler : register(s0);
