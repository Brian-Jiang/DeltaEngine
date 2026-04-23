module StandardLighting;

import StandardConstantStructs;

public struct LightResult
{
    public float4 Diffuse;
    public float4 Specular;
    public float4 Ambient;
};

float DoDiffuse(float3 N, float3 L)
{
    return max(0, dot(N, L));
}

float DoSpecular(float3 V, float3 N, float3 L, float specularPower)
{
    float3 H = normalize(L + V);
    float NdotH = max(0, dot(N, H));
    return pow(NdotH, specularPower);
}

public LightResult DoDirectionalLight(DirectionalLight light, float3 V, float3 P, float3 N, float specularPower)
{
    LightResult result;

    float3 L = normalize(-light.direction.xyz);

    result.Diffuse = light.color * light.intensity * DoDiffuse(N, L);
    result.Specular = light.color * light.intensity * DoSpecular(V, N, L, specularPower);
    result.Ambient = light.color * light.intensity * 0.1;

    return result;
}

public LightResult DoPointLight(PointLight light, float3 V, float3 P, float3 N, float specularPower)
{
    LightResult result = (LightResult)0;

    float3 toLight = light.position.xyz - P;
    float distance = length(toLight);
    if (distance > light.range)
        return result;

    float3 L = normalize(toLight);

    float falloff = saturate(1.0 - (distance / light.range) * (distance / light.range));
    falloff *= light.intensity;

    result.Diffuse = light.color * falloff * DoDiffuse(N, L);
    result.Specular = light.color * falloff * DoSpecular(V, N, L, specularPower);
    result.Ambient = float4(0, 0, 0, 0);

    return result;
}

public LightResult DoSpotLight(SpotLight light, float3 V, float3 P, float3 N, float specularPower)
{
    LightResult result = (LightResult)0;

    float3 toLight = light.position.xyz - P;
    float distance = length(toLight);
    if (distance > light.range)
        return result;

    float3 L = normalize(toLight);

    float theta = dot(-L, normalize(light.direction.xyz));
    float spotFactor = smoothstep(cos(light.outerConeAngle), cos(light.innerConeAngle), theta);
    if (spotFactor <= 0)
        return result;

    float distFalloff = saturate(1.0 - (distance / light.range) * (distance / light.range));
    float falloff = spotFactor * distFalloff * light.intensity;

    result.Diffuse = light.color * falloff * DoDiffuse(N, L);
    result.Specular = light.color * falloff * DoSpecular(V, N, L, specularPower);
    result.Ambient = float4(0, 0, 0, 0);

    return result;
}
