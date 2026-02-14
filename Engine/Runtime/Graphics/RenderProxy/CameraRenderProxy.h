#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

struct DXGraphicsContext;

class CameraRenderProxy
{
public:
    CameraRenderProxy();
    CameraRenderProxy(float fov, float aspectRatio, float nearPlane, float farPlane);
    void UpdateTransform(DirectX::XMMATRIX worldMatrix);
    void UpdateAspectRatio(float aspectRatio);
    void UpdateParameters(float fov, float aspectRatio, float nearPlane, float farPlane);
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