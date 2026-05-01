#include "DirectionalLightRenderProxy.h"

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/RenderProxy/CameraRenderProxy.h"
#include "Graphics/Shadow/ShadowView.h"
#include "Graphics/Structures/Light.h"

#include <DirectXMath.h>
#include <algorithm>
#include <cmath>

using namespace DeltaEngine;
using namespace DirectX;

namespace
{
    constexpr uint32_t kMaxDirShadowEdge = 2048u;
}

void DirectionalLightRenderProxy::UpdateParameters(XMVECTOR direction, XMVECTOR color, float intensity,
    bool castShadow, float shadowBias, float pcssLightSize, float shadowMaxDistance,
    float shadowOrthoPadding, int shadowResolution,
    float shadowNormalBias, float shadowSlopeBias, float shadowCasterDistance)
{
    m_direction = direction;
    m_color = color;
    m_intensity = intensity;
    m_castShadow = castShadow;
    m_shadowBias = shadowBias;
    m_pcssLightSize = pcssLightSize;
    m_shadowMaxDistance = shadowMaxDistance;
    m_shadowOrthoPadding = shadowOrthoPadding;
    m_shadowResolution = shadowResolution;
    m_shadowNormalBias = shadowNormalBias;
    m_shadowSlopeBias = shadowSlopeBias;
    m_shadowCasterDistance = shadowCasterDistance;
}

void DirectionalLightRenderProxy::SetDirectionalLightBufferIndex(uint32_t index)
{
    m_directionalLightBufferIndex = index;
}

void DirectionalLightRenderProxy::PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
    DirectionalLightBuffer lightData = {};
    lightData.direction = m_direction;
    lightData.color = m_color;
    lightData.intensity = m_intensity;
    lightData.lightViewProj = XMMatrixIdentity();
    lightData.atlasUVRect = { 0.0f, 0.0f, 1.0f, 1.0f };
    lightData.shadowBias = m_shadowBias;
    lightData.pcssLightSize = m_pcssLightSize;
    lightData.shadowEnabled = 0;
    lightData.shadowNormalBias = m_shadowNormalBias;
    lightData.shadowSlopeBias = m_shadowSlopeBias;

    renderContext->directionalLights.push_back(lightData);
}

void DirectionalLightRenderProxy::GatherShadowViews(std::shared_ptr<DXGraphicsContext> ctx, std::vector<ShadowView>& outViews)
{
    if (!m_castShadow || !ctx)
        return;

    float nearZ = 0.0f;
    float farEnd = 0.0f;
    float tanHalfFov = 0.0f;
    float aspect = 1.0f;
    XMMATRIX V_row = XMMatrixIdentity();

    if (ctx->activeRenderCamera.has_value())
    {
        const ActiveRenderCamera& arc = *ctx->activeRenderCamera;
        nearZ = arc.nearPlane;
        farEnd = arc.farPlane;
        tanHalfFov = std::tan(arc.fovY * 0.5f);
        aspect = arc.aspectRatio;
        V_row = XMMatrixTranspose(arc.cb.viewMatrix);
    }
    else if (ctx->camera)
    {
        const CameraRenderProxy* cam = ctx->camera;
        nearZ = cam->GetNearPlane();
        farEnd = cam->GetFarPlane();
        tanHalfFov = std::tan(cam->GetFov() * 0.5f);
        aspect = cam->GetAspectRatio();
        V_row = cam->GetViewMatrix();
    }
    else
        return;

    const float farClip = (std::min)(farEnd, m_shadowMaxDistance);
    if (farClip <= nearZ)
        return;

    XMVECTOR detV{};
    const XMMATRIX invV = XMMatrixInverse(&detV, V_row);
    if (std::fabs(XMVectorGetX(detV)) < 1e-12f)
        return;

    const float halfHNear = nearZ * tanHalfFov;
    const float halfWNear = halfHNear * aspect;
    const float halfHFar = farClip * tanHalfFov;
    const float halfWFar = halfHFar * aspect;

    XMVECTOR cornersCS[8];
    XMVECTOR cornersWS[8];
    XMVECTOR sumWS = XMVectorZero();
    for (int i = 0; i < 4; ++i)
    {
        const float sx = (i & 1) ? 1.0f : -1.0f;
        const float sy = (i & 2) ? 1.0f : -1.0f;
        cornersCS[i] = XMVectorSet(sx * halfWNear, sy * halfHNear, nearZ, 1.0f);
        cornersCS[i + 4] = XMVectorSet(sx * halfWFar, sy * halfHFar, farClip, 1.0f);
        cornersWS[i] = XMVector3TransformCoord(cornersCS[i], invV);
        cornersWS[i + 4] = XMVector3TransformCoord(cornersCS[i + 4], invV);
        sumWS = XMVectorAdd(sumWS, cornersWS[i]);
        sumWS = XMVectorAdd(sumWS, cornersWS[i + 4]);
    }

    if (nearZ != m_cachedNearZ || farClip != m_cachedFarClip ||
        tanHalfFov != m_cachedTanHalfFov || aspect != m_cachedAspect)
    {
        float maxDistSq = 0.0f;
        for (int i = 0; i < 8; ++i)
        {
            for (int j = i + 1; j < 8; ++j)
            {
                const XMVECTOR d = XMVectorSubtract(cornersCS[i], cornersCS[j]);
                const float dSq = XMVectorGetX(XMVector3LengthSq(d));
                if (dSq > maxDistSq)
                    maxDistSq = dSq;
            }
        }
        m_cachedFrustumRadius = 0.5f * std::sqrt(maxDistSq);
        m_cachedNearZ = nearZ;
        m_cachedFarClip = farClip;
        m_cachedTanHalfFov = tanHalfFov;
        m_cachedAspect = aspect;
    }
    const float radius = m_cachedFrustumRadius;

    const XMVECTOR centerWS = XMVectorScale(sumWS, 1.0f / 8.0f);
    const XMVECTOR lightDir = XMVector3Normalize(m_direction);

    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (std::fabs(XMVectorGetX(XMVector3Dot(lightDir, up))) > 0.99f)
        up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    const float eyeDistance = radius + m_shadowCasterDistance;
    const XMVECTOR eye = XMVectorSubtract(centerWS, XMVectorScale(lightDir, eyeDistance));
    const XMMATRIX lightView = XMMatrixLookToLH(eye, lightDir, up);

    const XMVECTOR centerLS = XMVector3TransformCoord(centerWS, lightView);
    const float cx = XMVectorGetX(centerLS);
    const float cy = XMVectorGetY(centerLS);
    const float cz = XMVectorGetZ(centerLS);

    const float pad = m_shadowOrthoPadding;
    m_dirMinX = cx - radius - pad;
    m_dirMaxX = cx + radius + pad;
    m_dirMinY = cy - radius - pad;
    m_dirMaxY = cy + radius + pad;
    m_dirMinZ = 0.0f;
    m_dirMaxZ = cz + radius;
    m_dirLightView = lightView;

    ShadowView sv{};
    sv.type = LightType::Directional;
    sv.lightIndex = m_directionalLightBufferIndex;
    sv.shadowParamsWriter = this;
    sv.viewProj = XMMatrixIdentity();
    const float q = (std::max)(0.05f, ctx->shadowQualityScalar);
    const uint32_t scaled = static_cast<uint32_t>(std::lround(static_cast<float>(m_shadowResolution) * q));
    const uint32_t edge = (std::clamp)(scaled, 32u, kMaxDirShadowEdge);
    sv.shadowMapEdgePx = edge;

    outViews.push_back(sv);
}

void DirectionalLightRenderProxy::FinishShadowViewProj(uint32_t resolutionW, uint32_t resolutionH, ShadowView& view)
{
    if (resolutionW == 0 || resolutionH == 0)
        return;

    const float spanX = m_dirMaxX - m_dirMinX;
    const float spanY = m_dirMaxY - m_dirMinY;
    if (spanX <= 1e-5f || spanY <= 1e-5f)
        return;

    const float texelW = spanX / static_cast<float>(resolutionW);
    const float texelH = spanY / static_cast<float>(resolutionH);

    const float minX = floorf(m_dirMinX / texelW) * texelW;
    const float minY = floorf(m_dirMinY / texelH) * texelH;
    const float maxX = minX + spanX;
    const float maxY = minY + spanY;

    const XMMATRIX ortho = XMMatrixOrthographicOffCenterLH(minX, maxX, minY, maxY, m_dirMinZ, m_dirMaxZ);
    m_shadowViewProjRow = m_dirLightView * ortho;
    view.viewProj = XMMatrixTranspose(m_shadowViewProjRow);
}

void DirectionalLightRenderProxy::WriteShadowParams(std::shared_ptr<DXGraphicsContext> ctx, const ShadowAllocation& alloc)
{
    if (!ctx || m_directionalLightBufferIndex >= ctx->directionalLights.size())
        return;

    DirectionalLightBuffer& L = ctx->directionalLights[m_directionalLightBufferIndex];
    L.lightViewProj = XMMatrixTranspose(m_shadowViewProjRow);
    L.atlasUVRect = alloc.atlasUVRect;
    L.shadowBias = m_shadowBias;
    L.pcssLightSize = m_pcssLightSize;
    L.shadowEnabled = m_castShadow ? 1 : 0;
    L.shadowNormalBias = m_shadowNormalBias;
    L.shadowSlopeBias = m_shadowSlopeBias;
}
