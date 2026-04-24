import IBL_Math;

struct SpecularCB
{
    uint  faceIndex;
    uint  mipLevel;
    uint  outputSize;
    float roughness;
    uint  sampleCount;
    uint  sourceCubeSize;
    uint  _pad0;
    uint  _pad1;
};

ConstantBuffer<SpecularCB> CB : register(b0);
TextureCube                g_src : register(t0);
RWTexture2DArray<float4>   g_dst : register(u0);
SamplerState               g_srcSampler : register(s0);

[shader("compute")]
[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= CB.outputSize || dtid.y >= CB.outputSize)
        return;

    float2 uv = (float2(dtid.xy) + 0.5) / float(CB.outputSize);
    float3 N  = DirectionFromCubeFaceUV(CB.faceIndex, uv);
    float3 R  = N;
    float3 V  = N;

    const uint SAMPLE_COUNT = CB.sampleCount;
    float3 prefiltered = float3(0, 0, 0);
    float  totalWeight = 0.0;

    float srcSize = float(CB.sourceCubeSize);
    float saTexel = 4.0 * IBL_PI / (6.0 * srcSize * srcSize);

    for (uint i = 0; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H  = ImportanceSampleGGX(Xi, N, CB.roughness);
        float3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            float NdotH = max(dot(N, H), 0.0);
            float VdotH = max(dot(V, H), 0.0);
            float D     = D_GGX_IBL(NdotH, CB.roughness);
            float pdf   = (D * NdotH / (4.0 * VdotH)) + 0.0001;

            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
            float mipLevel = (CB.roughness == 0.0) ? 0.0 : 0.5 * log2(saSample / saTexel);

            prefiltered += g_src.SampleLevel(g_srcSampler, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefiltered /= max(totalWeight, 0.0001);
    g_dst[uint3(dtid.xy, CB.faceIndex)] = float4(prefiltered, 1.0);
}
