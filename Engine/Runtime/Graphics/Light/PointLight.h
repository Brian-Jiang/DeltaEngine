#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Graphics/Light/LightComponent.h"

#include "PointLight.generated.h"

DELTA_ENGINE_NS_BEGIN

class PointLightRenderProxy;

DCLASS()
class PointLight : public LightComponent
{
    DGENERATED_BODY(PointLight)

public:
    PointLight();
    //PointLight(std::string name);
    //PointLight(std::string name, std::shared_ptr<GameObject> gameObject);
    ~PointLight();

    DFUNCTION()
    void UpdateParameters(DirectX::XMVECTOR color, float intensity, float range);

    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;

private:
    DPROPERTY()
    std::shared_ptr<PointLightRenderProxy> m_renderProxy;
    DPROPERTY()
    DirectX::XMVECTOR m_color;
    DPROPERTY()
    float m_intensity;
    DPROPERTY()
    float m_range;
};

DELTA_ENGINE_NS_END
