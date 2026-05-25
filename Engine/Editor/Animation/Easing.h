// Engine/Editor/Animation/Easing.h
#pragma once

#include <cmath>

namespace DeltaEngine
{

inline float CubicEaseOut(float t)
{
    return 1.0f - std::powf(1.0f - t, 3.0f);
}

}
