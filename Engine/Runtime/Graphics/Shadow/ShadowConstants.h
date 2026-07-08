#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

struct alignas(16) ShadowCBGPU
{
    int m_pcssBlockerSamples = 16;
    int m_pcssPCFSamples = 16;
    float m_qualityScalar = 1.0f;
    float m_penumbraGain = 8.0f;
    float m_penumbraScale = 3.0f;
    float m_minFilterTexels = 1.0f;
    float m_maxFilterTexels = 32.0f;
    float m_atlasTexelUv = 1.0f / 4096.0f;
};

DELTA_ENGINE_NS_END
