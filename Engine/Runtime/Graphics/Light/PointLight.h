#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Graphics/Light/LightComponent.h"

DELTA_ENGINE_NS_BEGIN

class PointLightRenderProxy;

class PointLight : public LightComponent
{
public:
    PointLight();
    PointLight(std::string name);
    PointLight(std::string name, std::shared_ptr<GameObject> gameObject);
    ~PointLight();

    void UpdateParameters(DirectX::XMVECTOR color, float intensity, float range);

    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;

private:
    std::shared_ptr<PointLightRenderProxy> m_renderProxy;
    DirectX::XMVECTOR m_color;
    float m_intensity;
    float m_range;
};

DELTA_ENGINE_NS_END
