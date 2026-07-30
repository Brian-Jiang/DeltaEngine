#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Runtime/Core/SceneComponent.h"

#include "LightComponent.generated.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;
struct DXGraphicsContext;
class RenderProxy;

DCLASS()
class LightComponent : public SceneComponent
{
    DGENERATED_BODY(LightComponent)
    friend class DWorld;

public:
    LightComponent() = default;

    virtual RenderProxy* GetRenderProxy() = 0;
    DELTAENGINE_API virtual void  SetIntensity(float intensity) = 0;
    DELTAENGINE_API virtual float GetIntensity() const = 0;

protected:
    virtual void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) = 0;

    DPROPERTY(meta=(Category="Shadows"))
    bool m_castShadow = true;

    DPROPERTY(meta=(Category="Shadows"))
    float m_shadowBias = 0.005f;

    DPROPERTY(meta=(Category="Shadows"))
    float m_shadowSlopeBias = 1.0f;

    DPROPERTY(meta=(Category="Shadows"))
    float m_shadowNormalBias = 0.0f;

    DPROPERTY(meta=(Category="Shadows"))
    float m_pcssLightSize = 0.05f;

    DPROPERTY(meta=(Category="Shadows"))
    int m_shadowResolution = 1024;
};

DELTA_ENGINE_NS_END
