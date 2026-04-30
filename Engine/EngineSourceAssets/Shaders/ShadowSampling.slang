module ShadowSampling;

import StandardConstantStructs;
import PCSS;

public float PCSS_Directional(float3 worldPos, DirectionalLight light)
{
    float4 clipPos = mul(light.lightViewProj, float4(worldPos, 1.0));
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

    float zReceiver = ndc.z - light.shadowBias;
    return PCSS_SoftShadow2D(g_shadowDirectionalAtlas, g_sampler, g_shadowCmpSampler, atlasUv, zReceiver,
        light.pcssLightSize, light.atlasUVRect.zw);
}

public float PCSS_Spot(float3 worldPos, SpotLight light)
{
    float4 clipPos = mul(light.lightViewProj, float4(worldPos, 1.0));
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

    float zReceiver = ndc.z - light.shadowBias;
    return PCSS_SoftShadow2D(g_shadowSpotAtlas, g_sampler, g_shadowCmpSampler, atlasUv, zReceiver,
        light.pcssLightSize, light.atlasUVRect.zw);
}
