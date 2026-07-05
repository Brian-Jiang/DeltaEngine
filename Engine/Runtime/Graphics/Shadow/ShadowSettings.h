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

    /** Gain applied to the receiver-blocker depth gap when estimating penumbra size; higher = softer sooner with distance. */
    DPROPERTY()
    float m_penumbraGain = 8.0f;

    /** How fast the PCF filter widens as the penumbra estimate grows. */
    DPROPERTY()
    float m_penumbraScale = 3.0f;

    /** Lower clamp on the PCF filter radius, in shadow-map texels; keeps contact shadows sharp. */
    DPROPERTY()
    float m_minFilterTexels = 1.0f;

    /** Upper clamp on the PCF filter radius, in shadow-map texels; stops the penumbra washing out the silhouette. */
    DPROPERTY()
    float m_maxFilterTexels = 12.0f;
};

DELTA_ENGINE_NS_END
