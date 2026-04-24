import StandardInputs;
import StandardLighting;
import StandardConstantStructs;

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
    float4 textureColor = g_matTextures[0].Sample(g_sampler, input.uv);
    float4 objectColor = input.color * textureColor;

    float3 V = normalize(CameraCB.position.xyz - input.worldPosition);
    float3 P = input.worldPosition;
    float3 N = normalize(input.normal);
    float specularPower = 128.0f;

    LightResult totalResult = (LightResult)0;

    for (uint i = 0; i < LightCB.numDirectionalLights; ++i)
    {
        LightResult r = DoDirectionalLight(DirectionalLights[i], V, P, N, specularPower);
        totalResult.Diffuse += r.Diffuse;
        totalResult.Specular += r.Specular;
        totalResult.Ambient += r.Ambient;
    }

    for (uint j = 0; j < LightCB.numPointLights; ++j)
    {
        LightResult r = DoPointLight(PointLights[j], V, P, N, specularPower);
        totalResult.Diffuse += r.Diffuse;
        totalResult.Specular += r.Specular;
    }

    for (uint k = 0; k < LightCB.numSpotLights; ++k)
    {
        LightResult r = DoSpotLight(SpotLights[k], V, P, N, specularPower);
        totalResult.Diffuse += r.Diffuse;
        totalResult.Specular += r.Specular;
    }

    float4 color = (totalResult.Ambient + totalResult.Diffuse) * objectColor + totalResult.Specular;
    return saturate(color);
}
