#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

/// Camera constant buffer layout (must match Shaders.hlsl Camera struct).
struct alignas(16) CameraCB
{
    DirectX::XMMATRIX viewMatrix;
    DirectX::XMMATRIX projectionMatrix;
    DirectX::XMVECTOR position;
};

DELTA_ENGINE_NS_END
