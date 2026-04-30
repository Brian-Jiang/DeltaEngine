#include "DirectionalLight.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/RenderProxy/DirectionalLightRenderProxy.h"

using namespace DeltaEngine;

DirectionalLight::DirectionalLight()
    : m_direction(0.0f, -1.0f, 0.0f, 0.0f)
    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
    , m_intensity(1.0f)
{
    m_renderProxy = std::make_shared<DirectionalLightRenderProxy>();
}

DirectionalLight::~DirectionalLight()
{
}

RenderProxy* DirectionalLight::GetRenderProxy()
{
    return m_renderProxy.get();
}

void DirectionalLight::UpdateParameters(DirectX::XMVECTOR direction, DirectX::XMVECTOR color, float intensity)
{
    m_direction = direction;
    m_color = color;
    m_intensity = intensity;
}

void DirectionalLight::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> context)
{
    DirectX::XMVECTOR direction = DirectX::XMVector4Normalize(DirectX::XMVectorSetW(GetForward(), 0.0f));
    m_renderProxy->UpdateParameters(direction, m_color, m_intensity);
    m_renderProxy->PreGatherDrawCalls(context);
}
