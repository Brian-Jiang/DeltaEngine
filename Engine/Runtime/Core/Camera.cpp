#include "Runtime/Core/Camera.h"

#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Graphics/RenderProxy/CameraRenderProxy.h"

using namespace DirectX;
using namespace DeltaEngine;

Camera::Camera()
    : m_fov(45.0f)
    , m_near(0.1f)
    , m_far(1000.0f)
    , m_aspectRatio(1.0f)
{
    m_renderProxy = std::make_shared<CameraRenderProxy>(m_fov, m_aspectRatio, m_near, m_far);
}

Camera::~Camera() = default;

void Camera::UpdateAspectRatio(float aspectRatio)
{
    if (!DELTA_ENSURE(aspectRatio > 0.0f))
        return;

    m_aspectRatio = aspectRatio;
    m_renderProxy->UpdateAspectRatio(aspectRatio);
}

void Camera::UpdateParameters(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    if (!DELTA_ENSURE(fov > 0.0f && fov < DirectX::XM_PI - 0.001f))
        return;
    if (!DELTA_ENSURE(aspectRatio > 0.0f))
        return;
    if (!DELTA_ENSURE(nearPlane > 0.0f && farPlane > nearPlane))
        return;

    m_fov = fov;
    m_aspectRatio = aspectRatio;
    m_near = nearPlane;
    m_far = farPlane;
    m_renderProxy->UpdateParameters(fov, aspectRatio, nearPlane, farPlane);
}

void Camera::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
    if (!DELTA_ENSURE(renderContext != nullptr))
        return;

    DELTA_ASSERT(m_renderProxy != nullptr);

    m_renderProxy->SetPostProcessStack(m_postProcessStack);
    renderContext->camera = m_renderProxy.get();
    m_renderProxy->PreGatherDrawCalls(renderContext);
}

void Camera::UpdateRenderProxy()
{
    m_renderProxy->UpdateParameters(m_fov, m_aspectRatio, m_near, m_far);
}

void Camera::OnTransformChanged()
{
    SceneComponent::OnTransformChanged();
    m_renderProxy->UpdateTransform(GetWorldTransform());
}
