module ShadowSampling;

import StandardConstantStructs;
import PCSS;

static const float kSlopeNdcScale = 1e-4;

static const float kSelfShadowNdcEpsilon = 1.5e-3;

float ComputeSlopeNdcBias(float NdotL, float slopeBias)
{
    float clampedN = max(NdotL, 0.05);
    float tanTerm = sqrt(saturate(1.0 - NdotL * NdotL)) / clampedN;
    return slopeBias * tanTerm * kSlopeNdcScale;
}

public float PCSS_Directional(float3 worldPos, float3 N, DirectionalLight light, float2 pixelCoord)
{
    float3 L_dir = normalize(-light.direction.xyz);
    float NdotL = saturate(dot(N, L_dir));
    float3 biased = worldPos - L_dir * light.shadowBias
        + N * light.shadowNormalBias * (1.0 - NdotL);

    float4 clipPos = mul(light.lightViewProj, float4(biased, 1.0));
    if (clipPos.w <= 0.0001)
        return 1.0;

    float3 ndc = clipPos.xyz / clipPos.w;
    if (ndc.z < 0.0 || ndc.z > 1.0 || ndc.x < -1.0 || ndc.x > 1.0 || ndc.y < -1.0 || ndc.y > 1.0)
        return 1.0;

    float2 ndcUv = float2(ndc.x * 0.5 + 0.5, -ndc.y * 0.5 + 0.5);
    float2 atlasUv = ndcUv * light.atlasUVRect.zw + light.atlasUVRect.xy;

    if (atlasUv.x <= light.atlasUVRect.x || atlasUv.y <= light.atlasUVRect.y ||
        atlasUv.x >= light.atlasUVRect.x + light.atlasUVRect.z ||
        atlasUv.y >= light.atlasUVRect.y + light.atlasUVRect.w)
        return 1.0;

    float zReceiver = ndc.z - ComputeSlopeNdcBias(NdotL, light.shadowSlopeBias) - kSelfShadowNdcEpsilon;
    return PCSS_SoftShadow2D(g_shadowDirectionalAtlas, g_sampler, g_shadowCmpSampler, atlasUv, zReceiver,
        light.pcssLightSize, light.atlasUVRect.zw, false, pixelCoord);
}

public float PCSS_Point(float3 worldPos, float3 N, PointLight light)
{
    if (light.cubeArrayIndex < 0)
        return 1.0;

    float3 toLight = light.position.xyz - worldPos;
    float distRaw = length(toLight);
    if (distRaw >= light.range)
        return 1.0;

    float3 L_dir = toLight / max(distRaw, 1e-5);
    float NdotL = saturate(dot(N, L_dir));
    float3 biased = worldPos - L_dir * light.shadowBias
        + N * light.shadowNormalBias * (1.0 - NdotL);

    float3 toFrag = biased - light.position.xyz;
    float dist = length(toFrag);
    if (dist >= light.range)
        return 1.0;

    float3 dir = toFrag / max(dist, 1e-5);

    float nearZ = light.shadowNearZ;
    float farZ = light.range;
    float zReceiver = farZ / (farZ - nearZ) - (farZ * nearZ) / ((farZ - nearZ) * dist);
    zReceiver -= ComputeSlopeNdcBias(NdotL, light.shadowSlopeBias) + kSelfShadowNdcEpsilon;

    float4 sampleCoord = float4(dir, (float)light.cubeArrayIndex);
    return g_shadowPointCubeArray.SampleCmpLevelZero(g_shadowCmpSampler, sampleCoord, zReceiver);
}

public float PCSS_Spot(float3 worldPos, float3 N, SpotLight light, float2 pixelCoord)
{
    float3 toLight = light.position.xyz - worldPos;
    float distRaw = length(toLight);
    float3 L_dir = toLight / max(distRaw, 1e-5);
    float NdotL = saturate(dot(N, L_dir));
    float3 biased = worldPos - L_dir * light.shadowBias
        + N * light.shadowNormalBias * (1.0 - NdotL);

    float4 clipPos = mul(light.lightViewProj, float4(biased, 1.0));
    if (clipPos.w <= 0.0001)
        return 1.0;

    float3 ndc = clipPos.xyz / clipPos.w;
    if (ndc.z < 0.0 || ndc.z > 1.0 || ndc.x < -1.0 || ndc.x > 1.0 || ndc.y < -1.0 || ndc.y > 1.0)
        return 1.0;

    float2 ndcUv = float2(ndc.x * 0.5 + 0.5, -ndc.y * 0.5 + 0.5);
    float2 atlasUv = ndcUv * light.atlasUVRect.zw + light.atlasUVRect.xy;

    if (atlasUv.x <= light.atlasUVRect.x || atlasUv.y <= light.atlasUVRect.y ||
        atlasUv.x >= light.atlasUVRect.x + light.atlasUVRect.z ||
        atlasUv.y >= light.atlasUVRect.y + light.atlasUVRect.w)
        return 1.0;

    float zReceiver = ndc.z - ComputeSlopeNdcBias(NdotL, light.shadowSlopeBias) - kSelfShadowNdcEpsilon;
    return PCSS_SoftShadow2D(g_shadowSpotAtlas, g_sampler, g_shadowCmpSampler, atlasUv, zReceiver,
        light.pcssLightSize, light.atlasUVRect.zw, true, pixelCoord);
}
