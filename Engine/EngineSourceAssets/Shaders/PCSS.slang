module PCSS;

import StandardConstantStructs;

static const int kMaxPoissonTaps = 32;

static const float2 kPoissonDisk[kMaxPoissonTaps] =
{
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725), float2(-0.09418410, -0.92938870),
    float2(0.34495938, 0.29387760), float2(-0.91588581, 0.45771432), float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379), float2(0.44323325, -0.97511554),
    float2(0.53742981, -0.47373420), float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590), float2(0.19984126, 0.78641367),
    float2(0.14383161, -0.14100790), float2(-0.06189810, 0.29524999), float2(-0.57800977, -0.79736776),
    float2(0.47964494, -0.90187502), float2(0.19083296, 0.62005602), float2(-0.07410704, 0.84446578),
    float2(0.37535738, -0.65827055), float2(0.06294723, 0.16563519), float2(0.94374807, -0.35743836),
    float2(0.86899725, 0.39541353), float2(-0.77866856, 0.10219533), float2(0.68187201, 0.59398156),
    float2(-0.29048823, 0.71102938), float2(0.42068579, 0.82028819), float2(0.37102109, -0.05106389),
    float2(-0.10665720, -0.79187044), float2(0.51385602, 0.54795743),
};

public float PCSS_BlockerSearch2D(Texture2D<float> shadowMap, SamplerState pointSampler, float2 uv, float zReceiver,
    float searchRadiusUv, int tapCount)
{
    float sumBlockers = 0.0;
    int count = 0;
    int n = clamp(tapCount, 1, kMaxPoissonTaps);
    for (int i = 0; i < n; ++i)
    {
        float2 suv = uv + kPoissonDisk[i] * searchRadiusUv;
        float d = shadowMap.SampleLevel(pointSampler, suv, 0.0).r;
        if (d < zReceiver)
        {
            sumBlockers += d;
            count++;
        }
    }
    if (count == 0)
        return -1.0;
    return sumBlockers / float(count);
}

public float PCSS_Filter2D(Texture2D<float> shadowMap, SamplerComparisonState cmpSampler, float2 uv, float zReceiver,
    float filterRadiusUv, int tapCount)
{
    float sum = 0.0;
    int n = clamp(tapCount, 1, kMaxPoissonTaps);
    for (int i = 0; i < n; ++i)
    {
        float2 suv = uv + kPoissonDisk[i] * filterRadiusUv;
        sum += shadowMap.SampleCmpLevelZero(cmpSampler, suv, zReceiver);
    }
    return sum / float(n);
}

public float PCSS_SoftShadow2D(Texture2D<float> shadowMap, SamplerState pointSampler, SamplerComparisonState cmpSampler,
    float2 atlasUv, float zReceiver, float lightAngularSize, float2 atlasScaleUv)
{
    int nBlock = ShadowCB.m_pcssBlockerSamples;
    int nPcf = ShadowCB.m_pcssPCFSamples;
    float q = ShadowCB.m_qualityScalar;
    float searchR = (0.0015 + lightAngularSize * 0.05) * q * max(atlasScaleUv.x, atlasScaleUv.y);

    float avgBlocker = PCSS_BlockerSearch2D(shadowMap, pointSampler, atlasUv, zReceiver, searchR, nBlock);
    if (avgBlocker < 0.0)
        return 1.0;

    float penumbra = saturate((zReceiver - avgBlocker) * 120.0 * q / max(avgBlocker, 1e-4));
    float filterR = searchR * (0.5 + penumbra * (2.0 + lightAngularSize * 20.0));
    return PCSS_Filter2D(shadowMap, cmpSampler, atlasUv, zReceiver, filterR, nPcf);
}
