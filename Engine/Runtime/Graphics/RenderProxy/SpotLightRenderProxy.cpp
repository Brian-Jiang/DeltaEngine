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
    // Note: outerConeAngle drives FOV = 2*outer for the shadow projection, so values >= XM_PIDIV2
    // are clamped at use site rather than rejected here (the default is XM_PIDIV2).
    if (!DELTA_ENSURE(outerConeAngle > 0.0f))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "SpotLightRenderProxy::UpdateParameters: outerConeAngle={} must be > 0; leaving previous values",
            outerConeAngle);
        return;
    }
    if (!DELTA_ENSURE(innerConeAngle >= 0.0f && innerConeAngle <= outerConeAngle))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "SpotLightRenderProxy::UpdateParameters: innerConeAngle={} out of range [0, outerConeAngle={}]; leaving previous values",
            innerConeAngle, outerConeAngle);
        return;
    }

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
    if (!DELTA_ENSURE(renderContext))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "SpotLightRenderProxy::PreGatherDrawCalls skipped: renderContext is null");
        return;
    }

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

void SpotLightRenderProxy::GatherShadowViews(std::shared_ptr<DXGraphicsContext> ctx, std::vector<ShadowView>& outViews)
{
    if (!m_castShadow)
        return;
    if (!DELTA_ENSURE(ctx))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "SpotLightRenderProxy::GatherShadowViews skipped: ctx is null");
        return;
    }
    if (m_range <= 0.0f)
        return;

    XMVECTOR dir = XMVector3Normalize(m_direction);
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (std::fabs(XMVectorGetX(XMVector3Dot(dir, up))) > 0.99f)
        up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    const XMMATRIX view = XMMatrixLookToLH(m_position, dir, up);
    const float nearZ = std::max(0.01f, std::min(m_range * 0.02f, 1.0f));
    // Clamp FOV strictly below pi to avoid a degenerate perspective matrix when outerConeAngle >= pi/2.
    const float fov = std::min(2.0f * m_outerConeAngle, DirectX::XM_PI - 0.01f);
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
    if (!DELTA_ENSURE(ctx))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "SpotLightRenderProxy::WriteShadowParams skipped: ctx is null");
        return;
    }
    if (!DELTA_ENSURE(m_spotLightBufferIndex < ctx->spotLights.size()))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "SpotLightRenderProxy::WriteShadowParams: light buffer index {} out of range (size={})",
            m_spotLightBufferIndex, ctx->spotLights.size());
        return;
    }

    SpotLightBuffer& L = ctx->spotLights[m_spotLightBufferIndex];
    L.lightViewProj = XMMatrixTranspose(m_shadowViewProjRow);
    L.atlasUVRect = alloc.atlasUVRect;
    L.shadowBias = m_shadowBias;
    L.pcssLightSize = m_pcssLightSize;
    L.shadowEnabled = m_castShadow ? 1 : 0;
    L.shadowNormalBias = m_shadowNormalBias;
    L.shadowSlopeBias = m_shadowSlopeBias;
}
