#pragma once

#include "EngineIncludes.h"

#include "ShadowSettings.generated.h"

DELTA_ENGINE_NS_BEGIN

DSTRUCT()
struct ShadowSettings
{
    DGENERATED_BODY_STRUCT(ShadowSettings)

    DPROPERTY()
    int m_pcssBlockerSamples = 16;

    DPROPERTY()
    int m_pcssPCFSamples = 16;

    DPROPERTY()
    float m_qualityScalar = 1.0f;
};

DELTA_ENGINE_NS_END
