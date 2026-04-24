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

//public Texture2D g_texture : register(t0, space1);

// ==== Sampler ====

public SamplerState g_sampler : register(s0);

// ==== Material (b3) ====

public struct Material
{
    public float4 baseColor;
    public float  metallic;
    public float  roughness;
    public float  emissiveIntensity;
    public float  alphaCutoff;
    public float4 emissiveColor;
    public uint   flags;
};

public ConstantBuffer<Material> MaterialCB : register(b3);

// ==== Material Textures (t0..t4, space1) ====

public Texture2D g_matTextures[5] : register(t0, space1);

// ==== Material Sampler ====

public SamplerState g_matSampler : register(s1);

// ==== Material Flags ====

public static const uint MAT_HAS_ALBEDO              = 1u << 0;
public static const uint MAT_HAS_NORMAL              = 1u << 1;
public static const uint MAT_HAS_METALLIC_ROUGHNESS  = 1u << 2;
public static const uint MAT_HAS_OCCLUSION           = 1u << 3;
public static const uint MAT_HAS_EMISSIVE            = 1u << 4;
public static const uint MAT_ALPHA_BLEND             = 1u << 5;
public static const uint MAT_ALPHA_TEST              = 1u << 6;
public static const uint MAT_DOUBLE_SIDED            = 1u << 7;

public bool MatHasFlag(uint flags, uint bit) { return (flags & bit) != 0u; }
