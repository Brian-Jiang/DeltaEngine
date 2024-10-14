#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

struct Light {
    DirectX::XMFLOAT3 position;
    float _padding;
    DirectX::XMFLOAT3 color;
    float intensity;
};

DELTA_ENGINE_NS_END