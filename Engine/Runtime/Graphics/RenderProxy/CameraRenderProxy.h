#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

struct DXGraphicsContext;
class PostProcessStack;

class CameraRenderProxy
{
public:
    CameraRenderProxy(float fov, float aspectRatio, float nearPlane, float farPlane);

    PostProcessStack* postProcessStack = nullptr;

    /// Updates the camera world transform.
    void UpdateTransform(DirectX::XMMATRIX worldMatrix);

    /// Updates the aspect ratio used to build the projection matrix.
    void UpdateAspectRatio(float aspectRatio);

    /// Updates the camera projection settings.
    void UpdateParameters(float fov, float aspectRatio, float nearPlane, float farPlane);

    /// Uploads the camera constant buffer for the current frame.
    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext);

private:
    void RecalculateViewProjectionMatrix();

private:
    float m_near;
    float m_far;
    float m_fov;
    float m_aspectRatio;
    DirectX::XMMATRIX m_worldMatrix;
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;
};

DELTA_ENGINE_NS_END
