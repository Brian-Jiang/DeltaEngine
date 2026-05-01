#include "SpotLightRenderProxy.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/Structures/Light.h"

#include <DirectXMath.h>
#include <cmath>

using namespace DeltaEngine;
using namespace DirectX;

void SpotLightRenderProxy::UpdateParameters(XMVECTOR position, XMVECTOR direction,
    XMVECTOR color, float intensity, float range,
    float innerConeAngle, float outerConeAngle,
    bool castShadow, float shadowBias, float pcssLightSize,
    float shadowNormalBias, float shadowSlopeBias)
{
    m_position = position;
    m_direction = direction;
    m_color = color;
    m_intensity = intensity;
    m_range = range;
    m_innerConeAngle = innerConeAngle;
    m_outerConeAngle = outerConeAngle;
    m_castShadow = castShadow;
    m_shadowBias = shadowBias;
    m_pcssLightSize = pcssLightSize;
    m_shadowNormalBias = shadowNormalBias;
    m_shadowSlopeBias = shadowSlopeBias;
}

void SpotLightRenderProxy::SetSpotLightBufferIndex(uint32_t index)
{
    m_spotLightBufferIndex = index;
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
    lightData.shadowBias = m_shadowBias;
    lightData.pcssLightSize = m_pcssLightSize;
    lightData.shadowEnabled = 0;
    lightData.shadowNormalBias = m_shadowNormalBias;
    lightData.shadowSlopeBias = m_shadowSlopeBias;

    renderContext->spotLights.push_back(lightData);
}

void SpotLightRenderProxy::GatherShadowViews(std::shared_ptr<DXGraphicsContext>, std::vector<ShadowView>& outViews)
{
    if (!m_castShadow || m_range <= 0.0f)
        return;

    XMVECTOR dir = XMVector3Normalize(m_direction);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (std::fabs(XMVectorGetX(XMVector3Dot(dir, up))) > 0.99f)
        up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    const XMMATRIX view = XMMatrixLookToLH(m_position, dir, up);
    const float nearZ = std::max(0.01f, std::min(m_range * 0.02f, 1.0f));
    const float fov = 2.0f * m_outerConeAngle;
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(fov, 1.0f, nearZ, m_range);
    m_shadowViewProjRow = view * proj;

    ShadowView sv{};
    sv.viewProj = XMMatrixTranspose(m_shadowViewProjRow);
    sv.type = LightType::Spot;
    sv.lightIndex = m_spotLightBufferIndex;
    sv.shadowParamsWriter = this;
    outViews.push_back(sv);
}

void SpotLightRenderProxy::WriteShadowParams(std::shared_ptr<DXGraphicsContext> ctx, const ShadowAllocation& alloc)
{
    if (!ctx || m_spotLightBufferIndex >= ctx->spotLights.size())
        return;

    SpotLightBuffer& L = ctx->spotLights[m_spotLightBufferIndex];
    L.lightViewProj = XMMatrixTranspose(m_shadowViewProjRow);
    L.atlasUVRect = alloc.atlasUVRect;
    L.shadowBias = m_shadowBias;
    L.pcssLightSize = m_pcssLightSize;
    L.shadowEnabled = m_castShadow ? 1 : 0;
    L.shadowNormalBias = m_shadowNormalBias;
    L.shadowSlopeBias = m_shadowSlopeBias;
}
