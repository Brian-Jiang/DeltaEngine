#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Graphics/Light/LightComponent.h"

DELTA_ENGINE_NS_BEGIN

class SpotLightRenderProxy;

class SpotLight : public LightComponent
{
public:
    SpotLight();
    SpotLight(std::string name);
    SpotLight(std::string name, std::shared_ptr<GameObject> gameObject);
    ~SpotLight();

    void UpdateParameters(DirectX::XMVECTOR color, float intensity, float range,
        float innerConeAngle, float outerConeAngle);

    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;

private:
    std::shared_ptr<SpotLightRenderProxy> m_renderProxy;
    DirectX::XMVECTOR m_color;
    float m_intensity;
    float m_range;
    float m_innerConeAngle; // radians
    float m_outerConeAngle; // radians
};

DELTA_ENGINE_NS_END
