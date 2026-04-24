import StandardConstantStructs;
import PBRInputs;
import PBRLighting;

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
float4 PSMain(PBRPSInput input) : SV_TARGET
{
    uint flags = MaterialCB.flags;

    float4 albedoSample = MatHasFlag(flags, MAT_HAS_ALBEDO)
        ? g_matTextures[0].Sample(g_matSampler, input.uv)
        : float4(1, 1, 1, 1);
    float3 baseColor = albedoSample.rgb * MaterialCB.baseColor.rgb * input.color.rgb;
    float  alpha     = albedoSample.a   * MaterialCB.baseColor.a   * input.color.a;

    if (MatHasFlag(flags, MAT_ALPHA_TEST))
    {
        if (alpha < MaterialCB.alphaCutoff)
            discard;
    }

    float2 mrSample = MatHasFlag(flags, MAT_HAS_METALLIC_ROUGHNESS)
        ? g_matTextures[2].Sample(g_matSampler, input.uv).gb
        : float2(MaterialCB.roughness, MaterialCB.metallic);
    float roughness = mrSample.x;
    float metallic  = mrSample.y;

    float3 N = normalize(input.normalWS);
    if (MatHasFlag(flags, MAT_HAS_NORMAL))
    {
        float3 T = normalize(input.tangentWS.xyz);
        float3 B = cross(N, T) * input.tangentWS.w;
        float3x3 TBN = float3x3(T, B, N);
        float3 nMap  = g_matTextures[1].Sample(g_matSampler, input.uv).xyz * 2.0 - 1.0;
        N = normalize(mul(nMap, TBN));
    }

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

    float3 V  = normalize(CameraCB.position.xyz - input.positionWS);
    float3 P  = input.positionWS;
    float3 Lo = float3(0, 0, 0);

    for (uint i = 0; i < LightCB.numDirectionalLights; ++i)
    {
        Lo += EvaluateDirectionalPBR(DirectionalLights[i], N, V, baseColor, metallic, roughness);
    }

    for (uint j = 0; j < LightCB.numPointLights; ++j)
    {
        Lo += EvaluatePointPBR(PointLights[j], P, N, V, baseColor, metallic, roughness);
    }

    for (uint k = 0; k < LightCB.numSpotLights; ++k)
    {
        Lo += EvaluateSpotPBR(SpotLights[k], P, N, V, baseColor, metallic, roughness);
    }

    float3 ambient = 0.03 * baseColor * ao;
    float3 color   = ambient + Lo + emissive;

    return float4(color, alpha);
}
