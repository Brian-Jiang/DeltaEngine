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
    float shadowOrthoPadding, int shadowResolution)
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

    XMVECTOR cornersWS[8];
    XMVECTOR sum = XMVectorZero();
    for (int i = 0; i < 4; ++i)
    {
        const float sx = (i & 1) ? 1.0f : -1.0f;
        const float sy = (i & 2) ? 1.0f : -1.0f;
        const XMVECTOR nearCorner = XMVectorSet(sx * halfWNear, sy * halfHNear, nearZ, 1.0f);
        const XMVECTOR farCorner = XMVectorSet(sx * halfWFar, sy * halfHFar, farClip, 1.0f);
        cornersWS[i] = XMVector3TransformCoord(nearCorner, invV);
        cornersWS[i + 4] = XMVector3TransformCoord(farCorner, invV);
        sum = XMVectorAdd(sum, cornersWS[i]);
        sum = XMVectorAdd(sum, cornersWS[i + 4]);
    }

    const XMVECTOR center = XMVectorScale(sum, 1.0f / 8.0f);
    const XMVECTOR lightDir = XMVector3Normalize(m_direction);

    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    if (std::fabs(XMVectorGetX(XMVector3Dot(lightDir, up))) > 0.99f)
        up = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

    const XMVECTOR eye = XMVectorSubtract(center, XMVectorScale(lightDir, 500.0f));
    const XMMATRIX lightView = XMMatrixLookToLH(eye, lightDir, up);

    float minX = 1e30f, maxX = -1e30f;
    float minY = 1e30f, maxY = -1e30f;
    float minZ = 1e30f, maxZ = -1e30f;

    for (int i = 0; i < 8; ++i)
    {
        const XMVECTOR ls = XMVector3TransformCoord(cornersWS[i], lightView);
        const float lx = XMVectorGetX(ls);
        const float ly = XMVectorGetY(ls);
        const float lz = XMVectorGetZ(ls);
        minX = (std::min)(minX, lx);
        maxX = (std::max)(maxX, lx);
        minY = (std::min)(minY, ly);
        maxY = (std::max)(maxY, ly);
        minZ = (std::min)(minZ, lz);
        maxZ = (std::max)(maxZ, lz);
    }

    const float pad = m_shadowOrthoPadding;
    m_dirMinX = minX - pad;
    m_dirMaxX = maxX + pad;
    m_dirMinY = minY - pad;
    m_dirMaxY = maxY + pad;
    m_dirMinZ = minZ - 1.0f;
    m_dirMaxZ = maxZ + 1.0f;
    m_dirLightView = lightView;

    ShadowView sv{};
    sv.type = LightType::Directional;
    sv.lightIndex = m_directionalLightBufferIndex;
    sv.shadowParamsWriter = this;
    sv.viewProj = XMMatrixIdentity();
    const uint32_t edge = (std::clamp)(static_cast<uint32_t>(m_shadowResolution), 32u, kMaxDirShadowEdge);
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
}
