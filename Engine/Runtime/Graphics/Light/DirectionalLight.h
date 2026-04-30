#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Graphics/Light/LightComponent.h"

#include "DirectionalLight.generated.h"

DELTA_ENGINE_NS_BEGIN

class DirectionalLightRenderProxy;

DCLASS()
class DirectionalLight : public LightComponent
{
    DGENERATED_BODY(DirectionalLight)

public:
    DirectionalLight();
    ~DirectionalLight();

    RenderProxy* GetRenderProxy() override;

    DFUNCTION()
    /// Updates the light parameters used for draw submission.
    void UpdateParameters(DirectX::XMVECTOR direction, DirectX::XMVECTOR color, float intensity);

    /// Uploads the current light data to the graphics context.
    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;

private:
    std::shared_ptr<DirectionalLightRenderProxy> m_renderProxy;

    DPROPERTY()
    DirectX::XMVECTOR m_direction;
    DPROPERTY(meta=(UIType="Color"))
    DirectX::XMVECTOR m_color;
    DPROPERTY()
    float m_intensity;

    DPROPERTY()
    float m_shadowMaxDistance = 100.0f;

    DPROPERTY()
    float m_shadowOrthoPadding = 5.0f;
};

DELTA_ENGINE_NS_END
