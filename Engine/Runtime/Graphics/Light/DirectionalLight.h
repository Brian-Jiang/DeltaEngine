#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Graphics/Light/LightComponent.h"

DELTA_ENGINE_NS_BEGIN

class DirectionalLightRenderProxy;

class DirectionalLight : public LightComponent
{
public:
    DirectionalLight();
    DirectionalLight(std::string name);
    DirectionalLight(std::string name, std::shared_ptr<GameObject> gameObject);
    ~DirectionalLight();

    void UpdateParameters(DirectX::XMVECTOR direction, DirectX::XMVECTOR color, float intensity);

    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;

private:
    std::shared_ptr<DirectionalLightRenderProxy> m_renderProxy;
    DirectX::XMVECTOR m_direction;
    DirectX::XMVECTOR m_color;
    float m_intensity;
};

DELTA_ENGINE_NS_END
