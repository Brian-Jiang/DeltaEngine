#include "SpotLightRenderProxy.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/Structures/Light.h"

using namespace DeltaEngine;

void SpotLightRenderProxy::UpdateParameters(DirectX::XMVECTOR position, DirectX::XMVECTOR direction,
    DirectX::XMVECTOR color, float intensity, float range,
    float innerConeAngle, float outerConeAngle)
{
    m_position = position;
    m_direction = direction;
    m_color = color;
    m_intensity = intensity;
    m_range = range;
    m_innerConeAngle = innerConeAngle;
    m_outerConeAngle = outerConeAngle;
}

void SpotLightRenderProxy::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
    SpotLightBuffer lightData = {};
    lightData.position = m_position;
    lightData.direction = m_direction;
    lightData.color = m_color;
    lightData.intensity = m_intensity;
    lightData.range = m_range;
    lightData.innerConeAngle = m_innerConeAngle;
    lightData.outerConeAngle = m_outerConeAngle;

    renderContext->spotLights.push_back(lightData);
}
