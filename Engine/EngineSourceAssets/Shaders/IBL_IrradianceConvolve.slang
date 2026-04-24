import IBL_Math;

struct IrradianceCB
{
    uint faceIndex;
    uint outputSize;
    uint _pad0;
    uint _pad1;
};

ConstantBuffer<IrradianceCB> CB : register(b0);
TextureCube                  g_src : register(t0);
RWTexture2DArray<float4>     g_dst : register(u0);
SamplerState                 g_srcSampler : register(s0);

[shader("compute")]
[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    if (dtid.x >= CB.outputSize || dtid.y >= CB.outputSize)
        return;

    float2 uv = (float2(dtid.xy) + 0.5) / float(CB.outputSize);
    float3 N  = DirectionFromCubeFaceUV(CB.faceIndex, uv);

    float3 up        = abs(N.y) < 0.999 ? float3(0, 1, 0) : float3(0, 0, 1);
    float3 right     = normalize(cross(up, N));
    up               = normalize(cross(N, right));

    const uint NUM_PHI   = 256u;
    const uint NUM_THETA = 64u;

    float3 irradiance = float3(0, 0, 0);
    float  sampleCount = 0.0;

    for (uint p = 0; p < NUM_PHI; ++p)
    {
        float phi = (float(p) + 0.5) * (2.0 * IBL_PI / float(NUM_PHI));
        for (uint t = 0; t < NUM_THETA; ++t)
        {
            float theta = (float(t) + 0.5) * (0.5 * IBL_PI / float(NUM_THETA));

            float3 tangentSample = float3(sin(theta) * cos(phi),
                                          sin(theta) * sin(phi),
                                          cos(theta));
            float3 sampleVec = tangentSample.x * right
                             + tangentSample.y * up
                             + tangentSample.z * N;

            irradiance += g_src.SampleLevel(g_srcSampler, sampleVec, 0).rgb
                        * cos(theta) * sin(theta);
            sampleCount += 1.0;
        }
    }

    irradiance = IBL_PI * irradiance / sampleCount;
    g_dst[uint3(dtid.xy, CB.faceIndex)] = float4(irradiance, 1.0);
}
