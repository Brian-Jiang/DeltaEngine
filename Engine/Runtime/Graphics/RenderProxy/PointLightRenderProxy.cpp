#include "PointLightRenderProxy.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/Structures/Light.h"

using namespace DeltaEngine;

void PointLightRenderProxy::UpdateParameters(DirectX::XMVECTOR position, DirectX::XMVECTOR color, float intensity, float range)
{
    m_position = position;
    m_color = color;
    m_intensity = intensity;
    m_range = range;
}

void PointLightRenderProxy::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
    PointLightBuffer lightData = {};
    lightData.position = m_position;
    lightData.color = m_color;
    lightData.intensity = m_intensity;
    lightData.range = m_range;

    renderContext->pointLights.push_back(lightData);
}
