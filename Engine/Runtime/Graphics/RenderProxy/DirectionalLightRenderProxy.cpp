#include "DirectionalLightRenderProxy.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/Structures/Light.h"

#include <DirectXMath.h>

using namespace DeltaEngine;
using namespace DirectX;

void DirectionalLightRenderProxy::UpdateParameters(DirectX::XMVECTOR direction, DirectX::XMVECTOR color, float intensity)
{
    m_direction = direction;
    m_color = color;
    m_intensity = intensity;
}

void DirectionalLightRenderProxy::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
    DirectionalLightBuffer lightData = {};
    lightData.direction = m_direction;
    lightData.color = m_color;
    lightData.intensity = m_intensity;
    lightData.lightViewProj = XMMatrixIdentity();
    lightData.atlasUVRect = { 0.0f, 0.0f, 1.0f, 1.0f };
    lightData.shadowEnabled = 0;

    renderContext->directionalLights.push_back(lightData);
}
