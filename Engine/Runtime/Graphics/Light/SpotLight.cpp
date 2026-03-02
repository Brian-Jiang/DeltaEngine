#include "SpotLight.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/RenderProxy/SpotLightRenderProxy.h"

using namespace DeltaEngine;

SpotLight::SpotLight()
    : m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_intensity(1.0f)
    , m_range(10.0f)
    , m_innerConeAngle(DirectX::XM_PIDIV4)   // 45 degrees
    , m_outerConeAngle(DirectX::XM_PIDIV2)   // 90 degrees
{
    m_renderProxy = std::make_shared<SpotLightRenderProxy>();
}

SpotLight::~SpotLight()
{
}

void SpotLight::UpdateParameters(DirectX::XMVECTOR color, float intensity, float range,
    float innerConeAngle, float outerConeAngle)
{
    m_color = color;
    m_intensity = intensity;
    m_range = range;
    m_innerConeAngle = innerConeAngle;
    m_outerConeAngle = outerConeAngle;
}

void SpotLight::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context)
{
    auto pos = GetWorldPosition();
    DirectX::XMVECTOR position = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
    DirectX::XMVECTOR direction = DirectX::XMVector4Normalize(DirectX::XMVectorSetW(GetForward(), 0.0f));

    m_renderProxy->UpdateParameters(position, direction, m_color, m_intensity, m_range,
        m_innerConeAngle, m_outerConeAngle);
    m_renderProxy->PreGatherDrawCalls(context);
}
