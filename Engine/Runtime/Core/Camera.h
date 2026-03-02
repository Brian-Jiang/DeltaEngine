#pragma once

#include "EngineIncludes.h"

#include <vector>

#include "SimpleMath.h"
#include "Core/SceneComponent.h"
#include "Graphics/Structures/Vertex.h"
#include "Core/DComponent.h"

#include "Camera.generated.h"

DELTA_ENGINE_NS_BEGIN

class CameraRenderProxy;
struct DXGraphicsContext;

DCLASS()
class Camera : public SceneComponent
{
    DGENERATED_BODY(Camera)

public:
    DELTAENGINE_API Camera();
    //DELTAENGINE_API Camera(std::string name);
    //DELTAENGINE_API Camera(std::string name, std::shared_ptr<GameObject> gameObject);
    DELTAENGINE_API ~Camera();

    DFUNCTION()
    DELTAENGINE_API void UpdateAspectRatio(float aspectRatio);

    DFUNCTION()
    DELTAENGINE_API void UpdateParameters(float fov, float aspectRatio, float nearPlane, float farPlane);

    DELTAENGINE_API void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext);

    //inline DirectX::XMMATRIX GetViewMatrix() const { return m_viewMatrix; }
    //inline DirectX::XMMATRIX GetProjectionMatrix() const { return m_projectionMatrix; }

protected:
    void OnTransformChanged() override;

private:
    DPROPERTY()
    float m_near;

    DPROPERTY()
    float m_far;

    DPROPERTY()
    float m_fov;

    DPROPERTY()
    float m_aspectRatio;

    std::shared_ptr<CameraRenderProxy> m_renderProxy;
};

DELTA_ENGINE_NS_END
