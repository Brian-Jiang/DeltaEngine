#pragma once

#include "EngineIncludes.h"

#include <vector>

#include "SimpleMath.h"
#include "Core/SceneComponent.h"
#include "Graphics/Structures/Vertex.h"
#include "Graphics/Texture.h"
#include "Core/Component.h"

DELTA_ENGINE_NS_BEGIN

class Camera : public SceneComponent
{
public:

    Camera();
    ~Camera();

    void Start(DirectX::XMFLOAT3 forward, DirectX::XMFLOAT3 up, float fov, float aspectRatio, float nearPlane, float farPlane);
    void Tick();

    DirectX::XMMATRIX GetViewMatrix() const { return m_viewMatrix; }
    DirectX::XMMATRIX GetProjectionMatrix() const { return m_projectionMatrix; }

protected:
    void OnTransformChanged() override;

private:
    float m_near;
    float m_far;
    float m_fov;
    float m_aspectRatio;
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_projectionMatrix;

    void RecalculateViewProjectionMatrix();
};

DELTA_ENGINE_NS_END
