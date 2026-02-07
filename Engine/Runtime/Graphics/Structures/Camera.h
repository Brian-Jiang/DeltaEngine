#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

/// Camera constant buffer layout (must match Shaders.hlsl Camera struct).
struct Camera
{
    DirectX::XMFLOAT4X4 viewMatrix;
    DirectX::XMFLOAT4X4 projectionMatrix;
    DirectX::XMFLOAT4 position;
};

DELTA_ENGINE_NS_END
