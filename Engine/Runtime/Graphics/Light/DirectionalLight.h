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
    //DirectionalLight(std::string name);
    //DirectionalLight(std::string name, std::shared_ptr<GameObject> gameObject);
    ~DirectionalLight();

    DFUNCTION()
    void UpdateParameters(DirectX::XMVECTOR direction, DirectX::XMVECTOR color, float intensity);

    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;

private:
    DPROPERTY()
    std::shared_ptr<DirectionalLightRenderProxy> m_renderProxy;
    DPROPERTY()
    DirectX::XMVECTOR m_direction;
    DPROPERTY()
    DirectX::XMVECTOR m_color;
    DPROPERTY()
    float m_intensity;
};

DELTA_ENGINE_NS_END
