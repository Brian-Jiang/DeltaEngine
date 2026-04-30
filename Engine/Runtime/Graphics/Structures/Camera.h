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

/// Per-frame active camera for rendering and shadow frustum fitting (editor preview or game camera).
struct alignas(16) ActiveRenderCamera
{
    CameraCB cb{};
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float aspectRatio = 1.0f;
    /// Vertical field of view in radians (see XMMatrixPerspectiveFovLH).
    float fovY = DirectX::XM_PIDIV4;
};

DELTA_ENGINE_NS_END
