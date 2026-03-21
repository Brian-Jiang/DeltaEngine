#pragma once

#include "EngineIncludes.h"

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
    case ViewportResolution::Resolution_1280x720:  outW = 1280;  outH = 720;  break;
    case ViewportResolution::Resolution_1920x1080: outW = 1920; outH = 1080; break;
    case ViewportResolution::Resolution_3840x2160: outW = 3840; outH = 2160; break;
    default: outW = 0; outH = 0; break;
    }
}

inline const char* GetResolutionPresetLabel(ViewportResolution preset)
{
    switch (preset)
    {
    case ViewportResolution::FreeAspect:           return "Free Aspect";
    case ViewportResolution::Resolution_1280x720:   return "1280 x 720";
    case ViewportResolution::Resolution_1920x1080: return "1920 x 1080";
    case ViewportResolution::Resolution_3840x2160: return "3840 x 2160 (4K)";
    default: return "Unknown";
    }
}

DELTA_ENGINE_NS_END
