#include "PointLightRenderProxy.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/Structures/Light.h"

#include <DirectXMath.h>
#include <algorithm>

using namespace DeltaEngine;
using namespace DirectX;

void PointLightRenderProxy::UpdateParameters(XMVECTOR position, XMVECTOR color,
    float intensity, float range,
    bool castShadow, float shadowBias, float pcssLightSize,
    float shadowNormalBias, float shadowSlopeBias)
{
    m_position = position;
    m_color = color;
    m_intensity = intensity;
    m_range = range;
    m_castShadow = castShadow;
    m_shadowBias = shadowBias;
    m_pcssLightSize = pcssLightSize;
    m_shadowNormalBias = shadowNormalBias;
    m_shadowSlopeBias = shadowSlopeBias;
}

void PointLightRenderProxy::SetPointLightBufferIndex(uint32_t index)
{
    m_pointLightBufferIndex = index;
}

void PointLightRenderProxy::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
    if (!DELTA_ENSURE(renderContext))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "PointLightRenderProxy::PreGatherDrawCalls skipped: renderContext is null");
        return;
    }

    PointLightBuffer lightData = {};
    lightData.position = m_position;
    lightData.color = m_color;
    lightData.intensity = m_intensity;
    lightData.range = m_range;
    lightData.cubeArrayIndex = -1;
    lightData.shadowBias = m_shadowBias;
    lightData.pcssLightSize = m_pcssLightSize;
    lightData.shadowEnabled = 0;
    lightData.shadowNearZ = m_shadowNearZ;
    lightData.shadowNormalBias = m_shadowNormalBias;
    lightData.shadowSlopeBias = m_shadowSlopeBias;

    renderContext->pointLights.push_back(lightData);
}

void PointLightRenderProxy::GatherShadowViews(std::shared_ptr<DXGraphicsContext> ctx, std::vector<ShadowView>& outViews)
{
    if (!m_castShadow)
        return;
    if (!DELTA_ENSURE(ctx))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "PointLightRenderProxy::GatherShadowViews skipped: ctx is null");
        return;
    }
    if (m_range <= 0.0f)
        return;

    // Cube face axes (LH, +X, -X, +Y, -Y, +Z, -Z) — D3D cube face order.
    static const XMVECTORF32 kForward[6] = {
        {  1.0f,  0.0f,  0.0f, 0.0f },
        { -1.0f,  0.0f,  0.0f, 0.0f },
        {  0.0f,  1.0f,  0.0f, 0.0f },
        {  0.0f, -1.0f,  0.0f, 0.0f },
        {  0.0f,  0.0f,  1.0f, 0.0f },
        {  0.0f,  0.0f, -1.0f, 0.0f },
    };
    static const XMVECTORF32 kUp[6] = {
        { 0.0f, 1.0f,  0.0f, 0.0f },
        { 0.0f, 1.0f,  0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { 0.0f, 0.0f,  1.0f, 0.0f },
        { 0.0f, 1.0f,  0.0f, 0.0f },
        { 0.0f, 1.0f,  0.0f, 0.0f },
    };

    m_shadowNearZ = std::max(0.01f, std::min(m_range * 0.02f, 1.0f));
    const XMMATRIX proj = XMMatrixPerspectiveFovLH(XM_PIDIV2, 1.0f, m_shadowNearZ, m_range);

    for (uint32_t face = 0; face < 6; ++face)
    {
        const XMMATRIX view = XMMatrixLookToLH(m_position, kForward[face], kUp[face]);
        const XMMATRIX viewProj = view * proj;

        ShadowView sv{};
        sv.viewProj = XMMatrixTranspose(viewProj);
        sv.type = LightType::Point;
        sv.lightIndex = m_pointLightBufferIndex;
        sv.cubeFace = face;
        sv.shadowParamsWriter = this;
        outViews.push_back(sv);
    }
}

void PointLightRenderProxy::WriteShadowParams(std::shared_ptr<DXGraphicsContext> ctx, const ShadowAllocation& alloc)
{
    if (!DELTA_ENSURE(ctx))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "PointLightRenderProxy::WriteShadowParams skipped: ctx is null");
        return;
    }
    if (!DELTA_ENSURE(m_pointLightBufferIndex < ctx->pointLights.size()))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "PointLightRenderProxy::WriteShadowParams: light buffer index {} out of range (size={})",
            m_pointLightBufferIndex, ctx->pointLights.size());
        return;
    }

    PointLightBuffer& L = ctx->pointLights[m_pointLightBufferIndex];
    L.cubeArrayIndex = alloc.cubeArrayIndex;
    L.shadowBias = m_shadowBias;
    L.pcssLightSize = m_pcssLightSize;
    L.shadowEnabled = m_castShadow ? 1 : 0;
    L.shadowNearZ = m_shadowNearZ;
    L.shadowNormalBias = m_shadowNormalBias;
    L.shadowSlopeBias = m_shadowSlopeBias;
}
