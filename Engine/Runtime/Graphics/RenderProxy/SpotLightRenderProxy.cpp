#include "SpotLightRenderProxy.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/Structures/Light.h"

#include <DirectXMath.h>

using namespace DeltaEngine;
using namespace DirectX;

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
    lightData.lightViewProj = XMMatrixIdentity();
    lightData.atlasUVRect = { 0.0f, 0.0f, 1.0f, 1.0f };
    lightData.shadowEnabled = 0;

    renderContext->spotLights.push_back(lightData);
}
