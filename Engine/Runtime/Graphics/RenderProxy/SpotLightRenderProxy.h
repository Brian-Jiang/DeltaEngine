#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

#include <memory>
#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

class DELTAENGINE_API SpotLightRenderProxy : public RenderProxy
{
public:
    void UpdateParameters(DirectX::XMVECTOR position, DirectX::XMVECTOR direction,
        DirectX::XMVECTOR color, float intensity, float range,
        float innerConeAngle, float outerConeAngle,
        bool castShadow, float shadowBias, float pcssLightSize,
        float shadowNormalBias, float shadowSlopeBias);
    void SetSpotLightBufferIndex(uint32_t index);
    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext) override;
    void GatherShadowViews(std::shared_ptr<DXGraphicsContext> ctx, std::vector<ShadowView>& outViews) override;
    void WriteShadowParams(std::shared_ptr<DXGraphicsContext> ctx, const ShadowAllocation& alloc) override;

private:
    DirectX::XMVECTOR m_position{};
    DirectX::XMVECTOR m_direction{};
    DirectX::XMVECTOR m_color{};
    float m_intensity = 1.0f;
    float m_range = 10.0f;
    float m_innerConeAngle = 0.0f;
    float m_outerConeAngle = 0.0f;
    bool m_castShadow = true;
    float m_shadowBias = 0.005f;
    float m_pcssLightSize = 0.05f;
    float m_shadowNormalBias = 0.0f;
    float m_shadowSlopeBias = 1.0f;
    uint32_t m_spotLightBufferIndex = 0;
    DirectX::XMMATRIX m_shadowViewProjRow = DirectX::XMMatrixIdentity();
};

DELTA_ENGINE_NS_END
