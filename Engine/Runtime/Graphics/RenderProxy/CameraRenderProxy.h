#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

#include <DirectXMath.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class PostProcessStack;

class CameraRenderProxy : public RenderProxy
{
public:
    DELTAENGINE_API CameraRenderProxy(float fov, float aspectRatio, float nearPlane, float farPlane);

    /// Returns the post-process stack the camera should drive (may be null).
    PostProcessStack* GetPostProcessStack() const { return m_postProcessStack; }
    /// Sets the post-process stack the camera should drive (may be null).
    void SetPostProcessStack(PostProcessStack* stack) { m_postProcessStack = stack; }

    /// Updates the camera world transform.
    DELTAENGINE_API void UpdateTransform(DirectX::XMMATRIX worldMatrix);

    /// Updates the aspect ratio used to build the projection matrix.
    DELTAENGINE_API void UpdateAspectRatio(float aspectRatio);

    /// Updates the camera projection settings.
    DELTAENGINE_API void UpdateParameters(float fov, float aspectRatio, float nearPlane, float farPlane);

    /// Uploads the camera constant buffer for the current frame.
    DELTAENGINE_API void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext) override;

    const DirectX::XMMATRIX& GetViewMatrix() const { return m_viewMatrix; }
    const DirectX::XMMATRIX& GetProjectionMatrix() const { return m_projectionMatrix; }
    float GetNearPlane() const { return m_near; }
    float GetFarPlane() const { return m_far; }
    float GetFov() const { return m_fov; }
    float GetAspectRatio() const { return m_aspectRatio; }

private:
    void RecalculateViewProjectionMatrix();

private:
    bool ValidateProjectionParams(float fov, float aspectRatio, float nearPlane, float farPlane) const;

private:
    float m_near = 0.1f;
    float m_far = 1000.0f;
    float m_fov = DirectX::XM_PIDIV4;
    float m_aspectRatio = 1.0f;
    DirectX::XMMATRIX m_worldMatrix = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX m_viewMatrix = DirectX::XMMatrixIdentity();
    DirectX::XMMATRIX m_projectionMatrix = DirectX::XMMatrixIdentity();
    PostProcessStack* m_postProcessStack = nullptr;
};

DELTA_ENGINE_NS_END
