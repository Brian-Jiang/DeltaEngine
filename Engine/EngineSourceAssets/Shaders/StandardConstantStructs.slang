module StandardConstantStructs;

// ==== CBV ====

// Camera (b0)
public struct Camera
{
    public float4x4 viewMatrix;
    public float4x4 projectionMatrix;
    public float4x4 invViewProjectionMatrix;
    public float4 position;
    public float4x4 projectionMatrixUnjittered;
    public float4x4 prevViewProjectionMatrix;
    public float2 jitter;
    public float2 jitterPad;
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
    public int cubeArrayIndex;
    public float shadowBias;
    public float pcssLightSize;
    public int shadowEnabled;
    public float shadowNearZ;
    public float shadowNormalBias;
    public float shadowSlopeBias;
    public float _pad1;
    public float _pad2;
    public float _pad3;
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
    public float4x4 lightViewProj;
    public float4 atlasUVRect;
    public float shadowBias;
    public float pcssLightSize;
    public int shadowEnabled;
    public float shadowNormalBias;
    public float shadowSlopeBias;
    public float _pad0;
    public float _pad1;
    public float _pad2;
};

public StructuredBuffer<SpotLight> SpotLights : register(t1);

public struct DirectionalLight
{
    public float4 direction;
    public float4 color;
    public float intensity;
    public float3 _padBeforeMatrix;
    public float4x4 lightViewProj;
    public float4 atlasUVRect;
    public float shadowBias;
    public float pcssLightSize;
    public int shadowEnabled;
    public float shadowNormalBias;
    public float shadowSlopeBias;
    public float _pad0;
    public float _pad1;
    public float _pad2;
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

public Texture2D g_matTextures[6] : register(t0, space1);

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
public static const uint MAT_HAS_ALPHA_MASK          = 1u << 8;

public bool MatHasFlag(uint flags, uint bit) { return (flags & bit) != 0u; }

// ==== IBL Resources (t0..t2, space2) ====

public TextureCube  g_irradianceMap : register(t0, space2);
public TextureCube  g_specularMap   : register(t1, space2);
public Texture2D    g_brdfLut       : register(t2, space2);

public SamplerState g_iblSampler : register(s2);

public struct ShadowGpuParams
{
    public int m_pcssBlockerSamples;
    public int m_pcssPCFSamples;
    public float m_qualityScalar;
    public float m_penumbraGain;
    public float m_penumbraScale;
    public float m_minFilterTexels;
    public float m_maxFilterTexels;
    public float m_atlasTexelUv;
};

public ConstantBuffer<ShadowGpuParams> ShadowCB : register(b4);

public Texture2D<float> g_shadowDirectionalAtlas : register(t0, space3);
public Texture2D<float> g_shadowSpotAtlas : register(t1, space3);
public TextureCubeArray<float> g_shadowPointCubeArray : register(t2, space3);

public SamplerComparisonState g_shadowCmpSampler : register(s3);

public static const float IBL_SPECULAR_MAX_MIP = 6.0;
