#pragma once

#include "EngineIncludes.h"

#include <type_traits>

DELTA_ENGINE_NS_BEGIN

/// Fixed output sizes for the viewport combo; FreeAspect uses the panel size instead.
enum class ViewportResolution
{
    FreeAspect,
    Resolution_1280x720,
    Resolution_1920x1080,
    Resolution_3840x2160,
    Count
};

inline void GetResolutionPresetSize(ViewportResolution preset, int& outW, int& outH)
{
    switch (preset)
    {
    case ViewportResolution::FreeAspect:
    case ViewportResolution::Count:
        outW = 0;
        outH = 0;
        break;
    case ViewportResolution::Resolution_1280x720:  outW = 1280;  outH = 720;  break;
    case ViewportResolution::Resolution_1920x1080: outW = 1920; outH = 1080; break;
    case ViewportResolution::Resolution_3840x2160: outW = 3840; outH = 2160; break;
    default:
        DELTA_UNREACHABLE();
    }
}

inline bool TryGetResolutionPresetSize(ViewportResolution preset, int& outW, int& outH)
{
    using U = std::underlying_type_t<ViewportResolution>;
    const U raw = static_cast<U>(preset);
    if (raw < 0 || raw >= static_cast<U>(ViewportResolution::Count))
        return false;
    GetResolutionPresetSize(preset, outW, outH);
    return true;
}

inline const char* GetResolutionPresetLabel(ViewportResolution preset)
{
    switch (preset)
    {
    case ViewportResolution::FreeAspect:           return "Free Aspect";
    case ViewportResolution::Resolution_1280x720:   return "1280 x 720";
    case ViewportResolution::Resolution_1920x1080: return "1920 x 1080";
    case ViewportResolution::Resolution_3840x2160: return "3840 x 2160 (4K)";
    case ViewportResolution::Count:                   return "Unknown";
    default:
        DELTA_UNREACHABLE();
    }
}

DELTA_ENGINE_NS_END
