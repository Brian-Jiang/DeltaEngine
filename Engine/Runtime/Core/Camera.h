#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Core/SceneComponent.h"

#include "Camera.generated.h"

DELTA_ENGINE_NS_BEGIN

class CameraRenderProxy;
class PostProcessStack;
struct DXGraphicsContext;

DCLASS()
class Camera : public SceneComponent
{
    DGENERATED_BODY(Camera)

public:
    DELTAENGINE_API Camera();
    DELTAENGINE_API ~Camera();

    /// Updates the projection aspect ratio.
    DFUNCTION()
    DELTAENGINE_API void UpdateAspectRatio(float aspectRatio);

    /// Updates the full projection parameter set.
    DFUNCTION()
    DELTAENGINE_API void UpdateParameters(float fov, float aspectRatio, float nearPlane, float farPlane);

    /// Writes this camera's frame data to the render context.
    DELTAENGINE_API void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext);

    /// Syncs cached camera settings to the render proxy.
    void UpdateRenderProxy();

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

    DPROPERTY()
    PostProcessStack* m_postProcessStack = nullptr;

    std::shared_ptr<CameraRenderProxy> m_renderProxy;
};

DELTA_ENGINE_NS_END
