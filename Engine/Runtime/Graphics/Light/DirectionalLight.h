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
    DELTAENGINE_API void UpdateParameters(DirectX::XMVECTOR color, float intensity);

    void SetIntensity(float intensity) override { m_intensity = intensity; }
    float GetIntensity() const override { return m_intensity; }

    /// Uploads the current light data to the graphics context.
    DELTAENGINE_API void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;

private:
    std::shared_ptr<DirectionalLightRenderProxy> m_renderProxy;

    DPROPERTY(meta=(UIType="Color"))
    DirectX::XMVECTOR m_color;
    DPROPERTY()
    float m_intensity;

    DPROPERTY(meta=(Category="Shadows"))
    float m_shadowMaxDistance = 100.0f;

    DPROPERTY(meta=(Category="Shadows"))
    float m_shadowOrthoPadding = 5.0f;

    /** Distance of caster space ahead of the frustum bounding sphere, along -lightDir (world units). */
    DPROPERTY(meta=(Category="Shadows"))
    float m_shadowCasterDistance = 5000.0f;
};

DELTA_ENGINE_NS_END
