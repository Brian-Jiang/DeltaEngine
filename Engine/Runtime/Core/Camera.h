#pragma once

#include "EngineIncludes.h"

#include <vector>

#include "SimpleMath.h"
#include "Core/SceneComponent.h"
#include "Graphics/Structures/Vertex.h"
#include "Core/DComponent.h"

DELTA_ENGINE_NS_BEGIN

class CameraRenderProxy;
struct DXGraphicsContext;

class Camera : public SceneComponent
{
public:

    Camera();
    Camera(std::string name);
    Camera(std::string name, std::shared_ptr<GameObject> gameObject);
    ~Camera();

    void UpdateAspectRatio(float aspectRatio);
    void UpdateParameters(float fov, float aspectRatio, float nearPlane, float farPlane);
    //void Tick();

    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext);

    //inline DirectX::XMMATRIX GetViewMatrix() const { return m_viewMatrix; }
    //inline DirectX::XMMATRIX GetProjectionMatrix() const { return m_projectionMatrix; }

protected:
    void OnTransformChanged() override;

private:
    float m_near;
    float m_far;
    float m_fov;
    float m_aspectRatio;
    //DirectX::XMMATRIX m_viewMatrix;
    //DirectX::XMMATRIX m_projectionMatrix;
    std::shared_ptr<CameraRenderProxy> m_renderProxy;

    //void RecalculateViewProjectionMatrix();
};

DELTA_ENGINE_NS_END
