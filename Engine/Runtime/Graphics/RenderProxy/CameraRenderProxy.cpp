#include "Graphics/RenderProxy/CameraRenderProxy.h"

#include "Graphics/Structures/Camera.h"
#include "Graphics/DXGraphicsContext.h"
#include "Graphics/DirectX/CommandList.h"

using namespace DeltaEngine;
using namespace DirectX;

CameraRenderProxy::CameraRenderProxy(float fov, float aspectRatio, float nearPlane, float farPlane)
    : m_fov(fov)
    , m_aspectRatio(aspectRatio)
    , m_near(nearPlane)
    , m_far(farPlane)
{
    RecalculateViewProjectionMatrix();
}

void DeltaEngine::CameraRenderProxy::UpdateTransform(DirectX::XMMATRIX worldMatrix)
{
    m_worldMatrix = worldMatrix;
    RecalculateViewProjectionMatrix();
}

void DeltaEngine::CameraRenderProxy::UpdateAspectRatio(float aspectRatio)
{
    m_aspectRatio = aspectRatio;
    RecalculateViewProjectionMatrix();
}

void DeltaEngine::CameraRenderProxy::UpdateParameters(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    m_fov = fov;
    m_aspectRatio = aspectRatio;
    m_near = nearPlane;
    m_far = farPlane;
    RecalculateViewProjectionMatrix();
}

void DeltaEngine::CameraRenderProxy::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
    CameraCB cameraData = {};
    cameraData.viewMatrix = XMMatrixTranspose(m_viewMatrix);
    cameraData.projectionMatrix = XMMatrixTranspose(m_projectionMatrix);
    cameraData.position = m_worldMatrix.r[3];

    std::shared_ptr<CommandList> commandList = renderContext->commandList;
    commandList->SetGraphicsDynamicConstantBuffer(0, cameraData);
}

void CameraRenderProxy::RecalculateViewProjectionMatrix()
{
    XMVECTOR forward = m_worldMatrix.r[2];
    XMVECTOR up = m_worldMatrix.r[1];
    XMVECTOR position = m_worldMatrix.r[3];
    m_viewMatrix = XMMatrixLookToLH(position, forward, up);
    m_projectionMatrix = XMMatrixPerspectiveFovLH(m_fov, m_aspectRatio, m_near, m_far);
}
