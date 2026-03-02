#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Graphics/Light/LightComponent.h"

#include "SpotLight.generated.h"

DELTA_ENGINE_NS_BEGIN

class SpotLightRenderProxy;

DCLASS()
    DELTAENGINE_API void Initialize(std::string name);
    DELTAENGINE_API void Initialize(std::string name, std::shared_ptr<GameObject> gameObject);
class SpotLight : public LightComponent
{
    DGENERATED_BODY(SpotLight)

public:
    SpotLight();
    SpotLight(std::string name) = delete;

    DELTAENGINE_API void Initialize(std::string name);

    SpotLight(std::string name, std::shared_ptr<GameObject> gameObject) = delete;

    DELTAENGINE_API void Initialize(std::string name, std::shared_ptr<GameObject> gameObject);

    ~SpotLight();

    DFUNCTION()
    void UpdateParameters(DirectX::XMVECTOR color, float intensity, float range,
        float innerConeAngle, float outerConeAngle);

    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;

private:
    DPROPERTY()
    std::shared_ptr<SpotLightRenderProxy> m_renderProxy;
    DPROPERTY()
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
