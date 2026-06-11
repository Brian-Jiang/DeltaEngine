import StandardConstantStructs;
import PBRInputs;

struct GBufferOutput
{
    float4 albedoAo : SV_Target0;
    float4 normal : SV_Target1;
    float4 material : SV_Target2;
    float3 emissive : SV_Target3;
};

[shader("vertex")]
PBRPSInput VSMain(PBRVSInput input)
{
    PBRPSInput result;

    float4 position      = float4(input.position, 1.0f);
    float4 worldPosition = mul(ObjectCB.worldMatrix, position);
    float4 viewPosition  = mul(CameraCB.viewMatrix, worldPosition);
    float4 clipPosition  = mul(CameraCB.projectionMatrix, viewPosition);

    float3 worldNormal  = normalize(mul((float3x3)ObjectCB.worldMatrix, input.normal));
    float3 worldTangent = normalize(mul((float3x3)ObjectCB.worldMatrix, input.tangent));

    result.position    = clipPosition;
    result.positionWS  = worldPosition.xyz;
    result.color       = input.color * ObjectCB.color;
    result.normalWS    = worldNormal;
    result.tangentWS   = float4(worldTangent, 1.0);
    result.uv          = input.uv;

    return result;
}

[shader("fragment")]
GBufferOutput PSMain(PBRPSInput input, bool isFrontFace : SV_IsFrontFace)
{
    uint flags = MaterialCB.flags;

    float4 albedoSample = MatHasFlag(flags, MAT_HAS_ALBEDO)
        ? g_matTextures[0].Sample(g_matSampler, input.uv)
        : float4(1, 1, 1, 1);
    float3 baseColor = albedoSample.rgb * MaterialCB.baseColor.rgb * input.color.rgb;
    float  alpha     = albedoSample.a   * MaterialCB.baseColor.a   * input.color.a;

    if (MatHasFlag(flags, MAT_HAS_ALPHA_MASK))
        alpha *= g_matTextures[5].Sample(g_matSampler, input.uv).r;

    if (MatHasFlag(flags, MAT_ALPHA_TEST))
    {
        if (alpha < MaterialCB.alphaCutoff)
            discard;
    }

    float roughness;
    float metallic;
    if (MatHasFlag(flags, MAT_HAS_METALLIC_ROUGHNESS))
    {
        float2 mrSample = g_matTextures[2].Sample(g_matSampler, input.uv).gb;
        roughness = mrSample.x * MaterialCB.roughness;
        metallic  = mrSample.y * MaterialCB.metallic;
    }
    else
    {
        roughness = MaterialCB.roughness;
        metallic  = MaterialCB.metallic;
    }

    float3 N = normalize(input.normalWS);
    if (MatHasFlag(flags, MAT_HAS_NORMAL))
    {
        float3 T = normalize(input.tangentWS.xyz);
        float3 B = cross(N, T) * input.tangentWS.w;
        float3x3 TBN = float3x3(T, B, N);
        float3 nMap  = g_matTextures[1].Sample(g_matSampler, input.uv).xyz * 2.0 - 1.0;
        N = normalize(mul(nMap, TBN));
    }

    if (MatHasFlag(flags, MAT_DOUBLE_SIDED) && !isFrontFace)
        N = -N;

    float ao = MatHasFlag(flags, MAT_HAS_OCCLUSION)
        ? g_matTextures[3].Sample(g_matSampler, input.uv).r
        : 1.0;

    float3 emissive = float3(0, 0, 0);
    if (MatHasFlag(flags, MAT_HAS_EMISSIVE))
    {
        emissive = g_matTextures[4].Sample(g_matSampler, input.uv).rgb
                 * MaterialCB.emissiveColor.rgb
                 * MaterialCB.emissiveIntensity;
    }

    GBufferOutput output;
    output.albedoAo = float4(baseColor, ao);
    output.normal = float4(N, 0.0);
    output.material = float4(metallic, roughness, asfloat(flags & 0xFFFFu), asfloat((flags >> 16) & 0xFFFFu));
    output.emissive = emissive;
    return output;
}
