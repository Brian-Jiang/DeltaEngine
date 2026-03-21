#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

/// Camera constant buffer layout (must match Shaders.hlsl Camera struct).
struct alignas(16) CameraCB
{
    /// View matrix for the active camera.
    DirectX::XMMATRIX viewMatrix;

    /// Projection matrix for the active camera.
    DirectX::XMMATRIX projectionMatrix;

    /// Camera world position.
    DirectX::XMVECTOR position;
};

DELTA_ENGINE_NS_END
