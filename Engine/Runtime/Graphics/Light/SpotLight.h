#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Graphics/Light/LightComponent.h"

#include "SpotLight.generated.h"

DELTA_ENGINE_NS_BEGIN

class SpotLightRenderProxy;
struct DXGraphicsContext;

DCLASS()
class SpotLight : public LightComponent
{
    DGENERATED_BODY(SpotLight)

public:
    SpotLight();
    ~SpotLight();

    RenderProxy* GetRenderProxy() override;

    DFUNCTION()
    /// Updates the light parameters used for draw submission.
    void UpdateParameters(DirectX::XMVECTOR color, float intensity, float range,
        float innerConeAngle, float outerConeAngle);

    /// Uploads the current light data to the graphics context.
    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;

private:
    std::shared_ptr<SpotLightRenderProxy> m_renderProxy;

    DPROPERTY(meta=(UIType="Color"))
    DirectX::XMVECTOR m_color;
    DPROPERTY()
    float m_intensity;
    DPROPERTY()
    float m_range;
    DPROPERTY()
    float m_innerConeAngle; // radians
    DPROPERTY()
    float m_outerConeAngle; // radians
};

DELTA_ENGINE_NS_END
