import StandardConstantStructs;
import PBRLighting;

Texture2D g_gbufferAlbedo : register(t0, space4);
Texture2D g_gbufferNormal : register(t1, space4);
Texture2D g_gbufferMaterial : register(t2, space4);
Texture2D g_gbufferEmissive : register(t3, space4);
Texture2D g_gbufferDepth : register(t4, space4);
SamplerState g_linearSampler : register(s0);

struct VSOut
{
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD0;
};

[shader("vertex")]
VSOut VSMain(uint id : SV_VertexID)
{
    float2 uv = float2((id << 1) & 2, id & 2);
    VSOut o;
    o.pos = float4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
    o.texCoord = float2(uv.x, 1.0f - uv.y);
    return o;
}

float3 ReconstructWorldPosition(float2 texCoord, float depth)
{
    float4 clipPos = float4(texCoord * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPosH = mul(CameraCB.invViewProjectionMatrix, clipPos);
    return worldPosH.xyz / worldPosH.w;
}

[shader("fragment")]
float4 PSMain(VSOut input) : SV_Target
{
    float4 albedoAo = g_gbufferAlbedo.Sample(g_linearSampler, input.texCoord);
    float4 normalData = g_gbufferNormal.Sample(g_linearSampler, input.texCoord);
    float4 materialData = g_gbufferMaterial.Sample(g_linearSampler, input.texCoord);
    float3 emissive = g_gbufferEmissive.Sample(g_linearSampler, input.texCoord).rgb;
    float depth = g_gbufferDepth.Sample(g_linearSampler, input.texCoord).r;

    float3 baseColor = albedoAo.rgb;
    float ao = albedoAo.a;
    float3 N = normalize(normalData.xyz);
    float metallic = materialData.x;
    float roughness = materialData.y;

    float3 P = ReconstructWorldPosition(input.texCoord, depth);
    float3 V = normalize(CameraCB.position.xyz - P);

    float3 Lo = float3(0, 0, 0);

    for (uint i = 0; i < LightCB.numDirectionalLights; ++i)
        Lo += EvaluateDirectionalPBR(DirectionalLights[i], P, N, V, baseColor, metallic, roughness);

    for (uint j = 0; j < LightCB.numPointLights; ++j)
        Lo += EvaluatePointPBR(PointLights[j], P, N, V, baseColor, metallic, roughness);

    for (uint k = 0; k < LightCB.numSpotLights; ++k)
        Lo += EvaluateSpotPBR(SpotLights[k], P, N, V, baseColor, metallic, roughness);

    float NdotV = max(dot(N, V), 0.0);
    float3 R = reflect(-V, N);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), baseColor, metallic);

    float3 irradiance = g_irradianceMap.Sample(g_iblSampler, N).rgb;
    float3 F_ambient = F_Schlick(NdotV, F0);
    float3 kD_ambient = (1.0 - F_ambient) * (1.0 - metallic);
    float3 diffuseIBL = kD_ambient * irradiance * baseColor;

    float lod = roughness * IBL_SPECULAR_MAX_MIP;
    float3 prefilteredColor = g_specularMap.SampleLevel(g_iblSampler, R, lod).rgb;
    float2 brdf = g_brdfLut.Sample(g_iblSampler, float2(NdotV, roughness)).rg;
    float3 specularIBL = prefilteredColor * (F_ambient * brdf.x + brdf.y);

    float3 ambient = (diffuseIBL + specularIBL) * ao;
    float3 color = ambient + Lo + emissive;

    return float4(color, 1.0f);
}
