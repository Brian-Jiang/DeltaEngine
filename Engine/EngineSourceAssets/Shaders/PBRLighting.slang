module PBRLighting;

import StandardConstantStructs;
import ShadowSampling;

static const float PBR_PI = 3.14159265358979f;

public float D_GGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
    return a2 / (PBR_PI * d * d);
}

public float G_SmithGGX(float NdotV, float NdotL, float roughness)
{
    float a    = roughness * roughness;
    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a) + a);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a) + a);
    return 0.5 / (GGXV + GGXL + 1e-5);
}

public float3 F_Schlick(float VdotH, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
}

public float3 F_SchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    float3 Fr = max((float3)(1.0 - roughness), F0);
    return F0 + (Fr - F0) * pow(1.0 - cosTheta, 5.0);
}

public float3 EvaluateDirectLight(
    float3 N, float3 V, float3 L,
    float3 lightColor, float lightAttenuation,
    float3 baseColor, float metallic, float roughness)
{
    float3 H     = normalize(V + L);
    float  NdotL = max(dot(N, L), 0.0);
    float  NdotV = max(dot(N, V), 1e-4);
    float  NdotH = max(dot(N, H), 0.0);
    float  VdotH = max(dot(V, H), 0.0);

    float3 F0 = lerp(float3(0.04, 0.04, 0.04), baseColor, metallic);
    float3 F  = F_Schlick(VdotH, F0);
    float  D  = D_GGX(NdotH, roughness);
    float  G  = G_SmithGGX(NdotV, NdotL, roughness);

    float3 specular = (D * G) * F;
    float3 kD       = (1.0 - F) * (1.0 - metallic);
    float3 diffuse  = kD * baseColor / PBR_PI;

    return (diffuse + specular) * lightColor * lightAttenuation * NdotL;
}

public float3 EvaluateDirectionalPBR(
    DirectionalLight light,
    float3 P, float3 N, float3 V,
    float3 baseColor, float metallic, float roughness)
{
    float3 L           = normalize(-light.direction.xyz);
    float3 lightColor  = light.color.rgb;
    float  attenuation = light.intensity;

    float vis = 1.0;
    if (light.shadowEnabled != 0)
        vis = PCSS_Directional(P, N, light);

    return EvaluateDirectLight(N, V, L, lightColor, attenuation, baseColor, metallic, roughness) * vis;
}

public float3 EvaluatePointPBR(
    PointLight light,
    float3 P, float3 N, float3 V,
    float3 baseColor, float metallic, float roughness)
{
    float3 toLight  = light.position.xyz - P;
    float  distance = length(toLight);
    if (distance > light.range)
        return float3(0, 0, 0);

    float3 L         = normalize(toLight);
    float  dr        = distance / light.range;
    float  falloff   = saturate(1.0 - dr * dr);
    float  attenuate = falloff * light.intensity;

    float vis = 1.0;
    if (light.shadowEnabled != 0)
        vis = PCSS_Point(P, N, light);

    return EvaluateDirectLight(N, V, L, light.color.rgb, attenuate, baseColor, metallic, roughness) * vis;
}

public float3 EvaluateSpotPBR(
    SpotLight light,
    float3 P, float3 N, float3 V,
    float3 baseColor, float metallic, float roughness)
{
    float3 toLight  = light.position.xyz - P;
    float  distance = length(toLight);
    if (distance > light.range)
        return float3(0, 0, 0);

    float3 L          = normalize(toLight);
    float  theta      = dot(-L, normalize(light.direction.xyz));
    float  spotFactor = smoothstep(cos(light.outerConeAngle), cos(light.innerConeAngle), theta);
    if (spotFactor <= 0.0)
        return float3(0, 0, 0);

    float  dr         = distance / light.range;
    float  distFall   = saturate(1.0 - dr * dr);
    float  attenuate  = spotFactor * distFall * light.intensity;

    float vis = 1.0;
    if (light.shadowEnabled != 0)
        vis = PCSS_Spot(P, N, light);

    return EvaluateDirectLight(N, V, L, light.color.rgb, attenuate, baseColor, metallic, roughness) * vis;
}
