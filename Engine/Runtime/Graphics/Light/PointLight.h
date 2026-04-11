#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Graphics/Light/LightComponent.h"

#include "PointLight.generated.h"

DELTA_ENGINE_NS_BEGIN

class PointLightRenderProxy;
struct DXGraphicsContext;

DCLASS()
class PointLight : public LightComponent
{
    DGENERATED_BODY(PointLight)

public:
    PointLight();
    ~PointLight();

    DFUNCTION()
    /// Updates the light parameters used for draw submission.
    void UpdateParameters(DirectX::XMVECTOR color, float intensity, float range);

    /// Uploads the current light data to the graphics context.
    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;

private:
    std::shared_ptr<PointLightRenderProxy> m_renderProxy;

    DPROPERTY(meta=(UIType="Color"))
    DirectX::XMVECTOR m_color;
    DPROPERTY()
    float m_intensity;
    DPROPERTY()
    float m_range;
};

DELTA_ENGINE_NS_END
