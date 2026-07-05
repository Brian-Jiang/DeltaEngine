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

// per-pixel rotation of the sample disk, seeded in screen space (interleaved gradient noise on the
// pixel coordinate) so neighbouring pixels decorrelate at pixel frequency and break up the
// texel-frequency penumbra banding into fine dither. returns (cos, sin).
float2 PCSS_RotationCosSin(float2 seed)
{
    float n = frac(52.9829189 * frac(dot(seed, float2(0.06711056, 0.00583715))));
    float a = n * 6.2831853;
    return float2(cos(a), sin(a));
}

float2 PCSS_RotateTap(float2 tap, float2 cs)
{
    return float2(tap.x * cs.x - tap.y * cs.y, tap.x * cs.y + tap.y * cs.x);
}

// get the average depth (blocker to light distance) of blockers in the search region, return -1 if no blockers found
public float PCSS_BlockerSearch2D(Texture2D<float> shadowMap, SamplerState pointSampler, float2 uv, float zReceiver,
    float searchRadiusUv, int tapCount, float2 rotCosSin)
{
    float sumBlockers = 0.0;
    int count = 0;
    int n = clamp(tapCount, 1, kMaxPoissonTaps);
    for (int i = 0; i < n; ++i)
    {
        float2 suv = uv + PCSS_RotateTap(kPoissonDisk[i], rotCosSin) * searchRadiusUv;
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

// get the fraction of samples that are lit in the filter region
public float PCSS_Filter2D(Texture2D<float> shadowMap, SamplerComparisonState cmpSampler, float2 uv, float zReceiver,
    float filterRadiusUv, int tapCount, float2 rotCosSin)
{
    float sum = 0.0;
    int n = clamp(tapCount, 1, kMaxPoissonTaps);
    for (int i = 0; i < n; ++i)
    {
        // sample compare the depth, considering interpolation on the shadow map
        float2 suv = uv + PCSS_RotateTap(kPoissonDisk[i], rotCosSin) * filterRadiusUv;
        sum += shadowMap.SampleCmpLevelZero(cmpSampler, suv, zReceiver);
    }
    return sum / float(n);
}

// perspective: true for spot/point (rays converge, penumbra normalized by blocker depth), false for
// directional (parallel rays, orthographic depth is already linear so no normalization)
public float PCSS_SoftShadow2D(Texture2D<float> shadowMap, SamplerState pointSampler, SamplerComparisonState cmpSampler,
    float2 atlasUv, float zReceiver, float lightAngularSize, float2 atlasScaleUv, bool perspective, float2 pixelCoord)
{
    int nBlock = ShadowCB.m_pcssBlockerSamples;
    int nPcf = ShadowCB.m_pcssPCFSamples;
    float q = ShadowCB.m_qualityScalar;

    // rotate the disk per pixel, seeded in screen space so within-texel pixels decorrelate
    float2 rotCosSin = PCSS_RotationCosSin(pixelCoord);

    // 0.0015 is the minimum search radius, 0.05 is the scale factor for light angular size, and q is the quality scalar
    float searchR = (0.0015 + lightAngularSize * 0.05) * q * max(atlasScaleUv.x, atlasScaleUv.y);

    float avgBlocker = PCSS_BlockerSearch2D(shadowMap, pointSampler, atlasUv, zReceiver, searchR, nBlock, rotCosSin);
    if (avgBlocker < 0.0)
        return 1.0;

    // zReceiver: the depth of the receiver in light space
    // avgBlocker: the average depth of blockers in light space, should be less than zReceiver because only closer blockers are considered
    // the receiver-blocker depth gap drives softness: small gap (contact) => hard, large gap => soft
    float depthGap = zReceiver - avgBlocker;
    float penumbra = perspective
        ? saturate(depthGap * ShadowCB.m_penumbraGain * q / max(avgBlocker, 1e-4))
        : saturate(depthGap * ShadowCB.m_penumbraGain * q);

    // 0.5 is the minimum filter radius; light angular size only feeds in once, through searchR
    float filterR = searchR * (0.5 + penumbra * ShadowCB.m_penumbraScale);

    // clamp the filter radius to a texel budget so the penumbra can't smear wider than the PCF tap count can resolve
    float texelUv = ShadowCB.m_atlasTexelUv;
    filterR = clamp(filterR, ShadowCB.m_minFilterTexels * texelUv, ShadowCB.m_maxFilterTexels * texelUv);

    return PCSS_Filter2D(shadowMap, cmpSampler, atlasUv, zReceiver, filterR, nPcf, rotCosSin);
}
