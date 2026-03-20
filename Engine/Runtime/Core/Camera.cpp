#include "Core/Camera.h"

#include "Graphics/RenderProxy/CameraRenderProxy.h"

using namespace DirectX;
using namespace DeltaEngine;

Camera::Camera()
    : m_fov(45.0f), m_near(0.1f), m_far(1000.0f), m_aspectRatio(1.0f)
{
    m_renderProxy = std::make_shared<CameraRenderProxy>(m_fov, m_aspectRatio, m_near, m_far);
}

//Camera::Camera(std::string name)
//    : SceneComponent(name), m_fov(45.0f), m_near(0.1f), m_far(1000.0f), m_aspectRatio(1.0f)
//{
//    m_renderProxy = std::make_shared<CameraRenderProxy>(m_fov, m_aspectRatio, m_near, m_far);
//}
//
//Camera::Camera(std::string name, std::shared_ptr<GameObject> gameObject)
//    : SceneComponent(name, gameObject), m_fov(45.0f), m_near(0.1f), m_far(1000.0f), m_aspectRatio(1.0f)
//{
//    m_renderProxy = std::make_shared<CameraRenderProxy>(m_fov, m_aspectRatio, m_near, m_far);
//}

Camera::~Camera()
{
}

void DeltaEngine::Camera::UpdateAspectRatio(float aspectRatio)
{
    m_aspectRatio = aspectRatio;
    m_renderProxy->UpdateAspectRatio(aspectRatio);
}

void Camera::UpdateParameters(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    m_fov = fov;
    m_aspectRatio = aspectRatio;
    m_near = nearPlane;
    m_far = farPlane;
    m_renderProxy->UpdateParameters(fov, aspectRatio, nearPlane, farPlane);
}

void Camera::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
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

//void DeltaEngine::Camera::RecalculateViewProjectionMatrix()
//{
//    // Recalculate the view projection matrix.
//    //XMVECTOR forward = GetForward();
//    //XMVECTOR up = GetUp();
//    //m_viewMatrix = XMMatrixLookToLH(GetWorldPosition(), forward, up);
//
//    //m_projectionMatrix = XMMatrixPerspectiveFovLH(m_fov, m_aspectRatio, m_near, m_far);
//}
