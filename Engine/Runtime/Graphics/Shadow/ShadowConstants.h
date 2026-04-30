#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

struct alignas(16) ShadowCBGPU
{
    int m_pcssBlockerSamples = 16;
    int m_pcssPCFSamples = 16;
    float m_qualityScalar = 1.0f;
    float _pad = 0.0f;
};

DELTA_ENGINE_NS_END
