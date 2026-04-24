import IBL_Math;

struct BrdfLutCB
{
    uint outputSize;
    uint sampleCount;
    uint _pad0;
    uint _pad1;
};

ConstantBuffer<BrdfLutCB> CB : register(b0);
RWTexture2D<float4>       g_dst : register(u0);

float2 IntegrateBRDF(float NdotV, float roughness)
{
    float3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    float3 N = float3(0, 0, 1);
    const uint SAMPLE_COUNT = CB.sampleCount;

    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, N, roughness);
        float3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G     = G_Smith_IBL(NdotV, NdotL, roughness);
            float G_Vis = (G * VdotH) / max(NdotH * NdotV, 1e-5);
            float Fc    = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    return float2(A, B) / float(SAMPLE_COUNT);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= CB.outputSize || dtid.y >= CB.outputSize)
        return;

    float2 uv = (float2(dtid.xy) + 0.5) / float(CB.outputSize);
    float  NdotV     = max(uv.x, 1e-4);
    float  roughness = max(uv.y, 1e-4);

    float2 v = IntegrateBRDF(NdotV, roughness);
    g_dst[dtid.xy] = float4(v.x, v.y, 0.0, 0.0);
}
