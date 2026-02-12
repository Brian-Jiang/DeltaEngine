#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

struct alignas(16) Light
{
    DirectX::XMVECTOR position;
    DirectX::XMVECTOR color;
    float intensity;
};

DELTA_ENGINE_NS_END