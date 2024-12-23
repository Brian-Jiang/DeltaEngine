#include "Core/Camera.h"

using namespace DirectX;
using namespace DeltaEngine;

Camera::Camera() 
    : m_fov(45.0f), m_near(0.1f), m_far(1000.0f), m_aspectRatio(1.0f), m_viewMatrix(XMMatrixIdentity()), m_projectionMatrix(XMMatrixIdentity())
{
    RecalculateViewProjectionMatrix();
}

void DeltaEngine::Camera::OnTransformChanged() {
    SceneComponent::OnTransformChanged();

    RecalculateViewProjectionMatrix();
}

void DeltaEngine::Camera::RecalculateViewProjectionMatrix() {
    // Recalculate the view projection matrix.
    XMVECTOR forward = GetForward();
    XMVECTOR up = GetUp();
    m_viewMatrix = XMMatrixLookToLH(GetWorldPosition(), forward, up);

    m_projectionMatrix = XMMatrixPerspectiveFovLH(m_fov, m_aspectRatio, m_near, m_far);
}
