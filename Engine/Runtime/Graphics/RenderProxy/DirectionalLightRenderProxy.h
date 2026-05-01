#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

#include <memory>
#include <DirectXMath.h>
#include <vector>

DELTA_ENGINE_NS_BEGIN

struct ShadowView;

class DELTAENGINE_API DirectionalLightRenderProxy : public RenderProxy
{
public:
    void UpdateParameters(DirectX::XMVECTOR direction, DirectX::XMVECTOR color, float intensity,
        bool castShadow, float shadowBias, float pcssLightSize, float shadowMaxDistance,
        float shadowOrthoPadding, int shadowResolution,
        float shadowNormalBias, float shadowSlopeBias);

    void SetDirectionalLightBufferIndex(uint32_t index);

    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext) override;
    void GatherShadowViews(std::shared_ptr<DXGraphicsContext> ctx, std::vector<ShadowView>& outViews) override;
    void WriteShadowParams(std::shared_ptr<DXGraphicsContext> ctx, const ShadowAllocation& alloc) override;

    void FinishShadowViewProj(uint32_t resolutionW, uint32_t resolutionH, ShadowView& view);

private:
    DirectX::XMVECTOR m_direction{};
    DirectX::XMVECTOR m_color{};
    float m_intensity = 1.0f;

    bool m_castShadow = true;
    float m_shadowBias = 0.005f;
    float m_pcssLightSize = 0.05f;
    float m_shadowNormalBias = 0.0f;
    float m_shadowSlopeBias = 1.0f;
    float m_shadowMaxDistance = 100.0f;
    float m_shadowOrthoPadding = 5.0f;
    int m_shadowResolution = 1024;

    uint32_t m_directionalLightBufferIndex = 0;
    DirectX::XMMATRIX m_shadowViewProjRow = DirectX::XMMatrixIdentity();

    DirectX::XMMATRIX m_dirLightView = DirectX::XMMatrixIdentity();
    float m_dirMinX = 0.0f;
    float m_dirMaxX = 0.0f;
    float m_dirMinY = 0.0f;
    float m_dirMaxY = 0.0f;
    float m_dirMinZ = 0.0f;
    float m_dirMaxZ = 0.0f;
};

DELTA_ENGINE_NS_END
