#include "PointLight.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/RenderProxy/PointLightRenderProxy.h"

using namespace DeltaEngine;

PointLight::PointLight()
    : m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_intensity(1.0f)
    , m_range(10.0f)
{
    m_renderProxy = std::make_shared<PointLightRenderProxy>();
}

PointLight::~PointLight()
{
}

RenderProxy* PointLight::GetRenderProxy()
{
    return m_renderProxy.get();
}

void PointLight::UpdateParameters(DirectX::XMVECTOR color, float intensity, float range)
{
    m_color = color;
    m_intensity = intensity;
    m_range = range;
}

void PointLight::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context)
{
    auto pos = GetWorldPosition();
    DirectX::XMVECTOR position = DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.0f);
    m_renderProxy->UpdateParameters(position, m_color, m_intensity, m_range,
        m_castShadow, m_shadowBias, m_pcssLightSize,
        m_shadowNormalBias, m_shadowSlopeBias);
    m_renderProxy->PreGatherDrawCalls(context);
    m_renderProxy->SetPointLightBufferIndex(static_cast<uint32_t>(context->pointLights.size() - 1));
}
