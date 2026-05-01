#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

#include <memory>
#include <DirectXMath.h>

DELTA_ENGINE_NS_BEGIN

class PointLightRenderProxy : public RenderProxy
{
public:
    void UpdateParameters(DirectX::XMVECTOR position, DirectX::XMVECTOR color,
        float intensity, float range,
        bool castShadow, float shadowBias, float pcssLightSize);
    void SetPointLightBufferIndex(uint32_t index);
    void PreGatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext) override;
    void GatherShadowViews(std::shared_ptr<DXGraphicsContext> ctx, std::vector<ShadowView>& outViews) override;
    void WriteShadowParams(std::shared_ptr<DXGraphicsContext> ctx, const ShadowAllocation& alloc) override;

private:
    DirectX::XMVECTOR m_position{};
    DirectX::XMVECTOR m_color{};
    float m_intensity = 1.0f;
    float m_range = 10.0f;
    bool m_castShadow = true;
    float m_shadowBias = 0.005f;
    float m_pcssLightSize = 0.05f;
    float m_shadowNearZ = 0.02f;
    uint32_t m_pointLightBufferIndex = 0;
};

DELTA_ENGINE_NS_END
