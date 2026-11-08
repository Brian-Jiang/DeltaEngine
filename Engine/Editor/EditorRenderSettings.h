#pragma once

#include "EditorIncludes.h"

#include <algorithm>

DELTA_ENGINE_NS_BEGIN

/// Editor-owned render toggles; EditorRenderManager applies them to the scene renderer before each frame.
class EditorRenderSettings
{
public:
    void SetTemporalAAEnabled(bool enabled) { m_temporalAA = enabled; }
    bool IsTemporalAAEnabled() const { return m_temporalAA; }

    /** Clamped to [0, 1]; 0 disables TAA sharpening. */
    void SetTemporalSharpness(float sharpness) { m_temporalSharpness = std::clamp(sharpness, 0.0f, 1.0f); }
    float GetTemporalSharpness() const { return m_temporalSharpness; }

    /** Discards TAA history on the next frame (camera cuts). */
    void RequestTemporalHistoryReset() { m_historyResetPending = true; }
    bool IsTemporalHistoryResetPending() const { return m_historyResetPending; }

    /** Returns true once per request. */
    bool ConsumeTemporalHistoryResetRequest()
    {
        const bool pending = m_historyResetPending;
        m_historyResetPending = false;
        return pending;
    }

private:
    bool m_temporalAA = false;
    float m_temporalSharpness = 0.0f;
    bool m_historyResetPending = false;
};

DELTA_ENGINE_NS_END
