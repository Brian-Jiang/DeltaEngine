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

//PointLight::PointLight(std::string name)
//    : LightComponent(name)
//    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
//    , m_intensity(1.0f)
//    , m_range(10.0f)
//{
//    m_renderProxy = std::make_shared<PointLightRenderProxy>();
//}
//
//PointLight::PointLight(std::string name, std::shared_ptr<GameObject> gameObject)
//    : LightComponent(name, gameObject)
//    , m_color(1.0f, 1.0f, 1.0f, 1.0f)
//    , m_intensity(1.0f)
//    , m_range(10.0f)
//{
//    m_renderProxy = std::make_shared<PointLightRenderProxy>();
//}

PointLight::~PointLight()
{
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
    m_renderProxy->UpdateParameters(position, m_color, m_intensity, m_range);
    m_renderProxy->PreGatherDrawCalls(context);
}
