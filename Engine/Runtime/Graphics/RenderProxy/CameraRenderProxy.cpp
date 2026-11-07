#include "Graphics/RenderProxy/CameraRenderProxy.h"

#include "Graphics/Structures/Camera.h"
#include "Graphics/DXGraphicsContext.h"
#include "Graphics/DirectX/CommandList.h"
#include "Runtime/Logging/LogChannels.h"

using namespace DeltaEngine;
using namespace DirectX;

CameraRenderProxy::CameraRenderProxy(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    if (ValidateProjectionParams(fov, aspectRatio, nearPlane, farPlane))
    {
        m_fov = fov;
        m_aspectRatio = aspectRatio;
        m_near = nearPlane;
        m_far = farPlane;
    }
    RecalculateViewProjectionMatrix();
}

bool CameraRenderProxy::ValidateProjectionParams(float fov, float aspectRatio, float nearPlane, float farPlane) const
{
    if (!DELTA_ENSURE(fov > 0.0f))
    {
        DLOG(LogRenderer, ELogLevel::Warning, "CameraRenderProxy: invalid fov={} (expected > 0)", fov);
        return false;
    }
    if (!DELTA_ENSURE(aspectRatio > 0.0f))
    {
        DLOG(LogRenderer, ELogLevel::Warning, "CameraRenderProxy: invalid aspectRatio={} (expected > 0)", aspectRatio);
        return false;
    }
    if (!DELTA_ENSURE(nearPlane > 0.0f))
    {
        DLOG(LogRenderer, ELogLevel::Warning, "CameraRenderProxy: invalid nearPlane={} (expected > 0)", nearPlane);
        return false;
    }
    if (!DELTA_ENSURE(farPlane > nearPlane))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "CameraRenderProxy: invalid farPlane={} (expected > nearPlane={})", farPlane, nearPlane);
        return false;
    }
    return true;
}

void CameraRenderProxy::UpdateTransform(DirectX::XMMATRIX worldMatrix)
{
    m_worldMatrix = worldMatrix;
    RecalculateViewProjectionMatrix();
}

void CameraRenderProxy::UpdateAspectRatio(float aspectRatio)
{
    if (!ValidateProjectionParams(m_fov, aspectRatio, m_near, m_far))
        return;
    m_aspectRatio = aspectRatio;
    RecalculateViewProjectionMatrix();
}

void CameraRenderProxy::UpdateParameters(float fov, float aspectRatio, float nearPlane, float farPlane)
{
    if (!ValidateProjectionParams(fov, aspectRatio, nearPlane, farPlane))
        return;
    m_fov = fov;
    m_aspectRatio = aspectRatio;
    m_near = nearPlane;
    m_far = farPlane;
    RecalculateViewProjectionMatrix();
}

void CameraRenderProxy::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
    if (!DELTA_ENSURE(renderContext && renderContext->commandList))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "CameraRenderProxy::PreGatherDrawCalls skipped: renderContext or commandList is null");
        return;
    }

    CameraCB cameraData = {};
    cameraData.viewMatrix = XMMatrixTranspose(m_viewMatrix);
    cameraData.projectionMatrix = XMMatrixTranspose(m_projectionMatrix);
    cameraData.position = m_worldMatrix.r[3];
    PopulateInvViewProjection(cameraData);

    renderContext->commandList->SetGraphicsDynamicConstantBuffer(0, cameraData);
}

void CameraRenderProxy::RecalculateViewProjectionMatrix()
{
    XMVECTOR forward = m_worldMatrix.r[2];
    XMVECTOR up = m_worldMatrix.r[1];
    XMVECTOR position = m_worldMatrix.r[3];
    m_viewMatrix = XMMatrixLookToLH(position, forward, up);
    m_projectionMatrix = XMMatrixPerspectiveFovLH(m_fov, m_aspectRatio, m_near, m_far);
}
