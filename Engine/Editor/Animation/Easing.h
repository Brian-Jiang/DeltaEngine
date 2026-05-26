// Engine/Editor/Animation/Easing.h
#pragma once

#include "EditorIncludes.h"

#include <cmath>

DELTA_ENGINE_NS_BEGIN

inline float CubicEaseOut(float t)
{
    return 1.0f - std::powf(1.0f - t, 3.0f);
}

DELTA_ENGINE_NS_END
