#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

/// Camera constant buffer layout (must match StandardConstantStructs.slang Camera / Skybox CameraCB).
struct alignas(16) CameraCB
{
    /// View matrix for the active camera.
    DirectX::XMMATRIX viewMatrix;

    /// Projection matrix for the active camera (jittered when temporal jitter is enabled).
    DirectX::XMMATRIX projectionMatrix;

    /// Inverse of (view * projection), transposed for HLSL mul(rowVec, matrix).
    DirectX::XMMATRIX invViewProjectionMatrix;

    /// Camera world position.
    DirectX::XMVECTOR position;

    /// Centered (unjittered) projection, transposed.
    DirectX::XMMATRIX projectionMatrixUnjittered;

    /// Previous frame unjittered view*projection, transposed.
    DirectX::XMMATRIX prevViewProjectionMatrix;

    /// Sub-pixel NDC jitter applied to projection this frame (x, y).
    DirectX::XMFLOAT2 jitter{};
    DirectX::XMFLOAT2 jitterPad{};
};

/// Computes and stores invViewProjectionMatrix from the transposed view/projection in cb.
inline void PopulateInvViewProjection(CameraCB& cb)
{
    const DirectX::XMMATRIX view = DirectX::XMMatrixTranspose(cb.viewMatrix);
    const DirectX::XMMATRIX projection = DirectX::XMMatrixTranspose(cb.projectionMatrix);
    const DirectX::XMMATRIX viewProjection = DirectX::XMMatrixMultiply(view, projection);
    cb.invViewProjectionMatrix = DirectX::XMMatrixTranspose(DirectX::XMMatrixInverse(nullptr, viewProjection));
}

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
