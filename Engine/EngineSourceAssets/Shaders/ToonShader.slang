import StandardInputs;
import StandardConstantStructs;

struct ToonLightResult
{
    float4 Diffuse;
    float4 Specular;
    float4 Ambient;
    float4 Rim;
};

float ToonDiffuse(float3 N, float3 L)
{
    float NdotL = max(0, dot(N, L));
    float bands = 3.0f;
    return floor(NdotL * bands) / bands;
}

float ToonSpecular(float3 V, float3 N, float3 L, float specularPower)
{
    float3 H = normalize(L + V);
    float NdotH = max(0, dot(N, H));
    float s = pow(NdotH, specularPower);
    return step(0.5, s);
}

float ToonRim(float3 V, float3 N, float3 L, float rimThreshold, float rimPower)
{
    float VdotN = dot(V, N);
    float NdotL = dot(N, L);
    float rim = 1.0 - VdotN;
    float rimValue = pow(rim, rimPower);
    return step(rimThreshold, rimValue * NdotL);
}

ToonLightResult DoDirectionalToon(DirectionalLight light, float3 V, float3 P, float3 N, float specularPower)
{
    ToonLightResult result;
    float3 L = normalize(-light.direction.xyz);
    result.Diffuse = light.color * light.intensity * ToonDiffuse(N, L);
    result.Specular = light.color * light.intensity * ToonSpecular(V, N, L, specularPower);
    result.Ambient = light.color * light.intensity * 0.1;
    result.Rim = light.color * light.intensity * ToonRim(V, N, L, 0.5, 4.0);
    return result;
}

ToonLightResult DoPointToon(PointLight light, float3 V, float3 P, float3 N, float specularPower)
{
    ToonLightResult result = (ToonLightResult)0;

    float3 toLight = light.position.xyz - P;
    float distance = length(toLight);
    if (distance > light.range)
        return result;

    float3 L = normalize(toLight);
    float falloff = saturate(1.0 - (distance / light.range) * (distance / light.range));
    falloff *= light.intensity;

    result.Diffuse = light.color * falloff * ToonDiffuse(N, L);
    result.Specular = light.color * falloff * ToonSpecular(V, N, L, specularPower);
    return result;
}

ToonLightResult DoSpotToon(SpotLight light, float3 V, float3 P, float3 N, float specularPower)
{
    ToonLightResult result = (ToonLightResult)0;

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

    result.Diffuse = light.color * falloff * ToonDiffuse(N, L);
    result.Specular = light.color * falloff * ToonSpecular(V, N, L, specularPower);
    return result;
}

[shader("vertex")]
StandardPSInput VSMain(StandardVSInput input)
{
    StandardPSInput result;

    float4 position = float4(input.position, 1.0f);
    float4 worldPosition = mul(ObjectCB.worldMatrix, position);
    float4 viewPosition = mul(CameraCB.viewMatrix, worldPosition);
    float4 clipPosition = mul(CameraCB.projectionMatrix, viewPosition);

    float3 worldNormal = mul((float3x3)ObjectCB.worldMatrix, input.normal);

    result.position = clipPosition;
    result.worldPosition = worldPosition.xyz;
    result.color = input.color * ObjectCB.color;
    result.normal = worldNormal;
    result.uv = input.uv;

    return result;
}

[shader("fragment")]
float4 PSMain(StandardPSInput input) : SV_TARGET
{
    float4 textureColor = g_texture.Sample(g_sampler, input.uv);
    float4 objectColor = input.color * textureColor;

    float3 V = normalize(CameraCB.position.xyz - input.worldPosition);
    float3 P = input.worldPosition;
    float3 N = normalize(input.normal);
    float specularPower = 256.0f;

    ToonLightResult total = (ToonLightResult)0;

    for (uint i = 0; i < LightCB.numDirectionalLights; ++i)
    {
        ToonLightResult r = DoDirectionalToon(DirectionalLights[i], V, P, N, specularPower);
        total.Diffuse += r.Diffuse;
        total.Specular += r.Specular * 0.5f;
        total.Ambient += r.Ambient;
        total.Rim += r.Rim;
    }

    for (uint j = 0; j < LightCB.numPointLights; ++j)
    {
        ToonLightResult r = DoPointToon(PointLights[j], V, P, N, specularPower);
        total.Diffuse += r.Diffuse;
        total.Specular += r.Specular;
    }

    for (uint k = 0; k < LightCB.numSpotLights; ++k)
    {
        ToonLightResult r = DoSpotToon(SpotLights[k], V, P, N, specularPower);
        total.Diffuse += r.Diffuse;
        total.Specular += r.Specular;
    }

    float4 color = (total.Ambient + total.Diffuse + total.Rim) * objectColor + total.Specular;
    return saturate(color);
}
