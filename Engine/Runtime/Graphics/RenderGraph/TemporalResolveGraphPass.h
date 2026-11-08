#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"
#include "Runtime/Graphics/TemporalResolvePipeline.h"

#include <d3d12.h>
#include <d3dx12.h>

DELTA_ENGINE_NS_BEGIN

/// Blends the current frame into the TAA history along motion vectors (TAA.slang).
class DELTAENGINE_API TemporalResolveGraphPass final : public RenderGraphPass
{
public:
    struct Inputs
    {
        RenderGraphTextureHandle color;
        RenderGraphTextureHandle historyRead;
        RenderGraphTextureHandle historyWrite;
        RenderGraphTextureHandle motionVectors;
        RenderGraphTextureHandle depth;
        D3D12_CPU_DESCRIPTOR_HANDLE colorSrv{};
        D3D12_CPU_DESCRIPTOR_HANDLE historySrv{};
        D3D12_CPU_DESCRIPTOR_HANDLE motionSrv{};
        D3D12_CPU_DESCRIPTOR_HANDLE depthSrv{};
        D3D12_CPU_DESCRIPTOR_HANDLE historyRtv{};
    };

    TemporalResolveGraphPass(const Inputs& inputs, const TemporalResolveConstants& constants,
        CD3DX12_VIEWPORT viewport, D3D12_RECT scissorRect, const TemporalResolvePipeline* pipeline);

    const char* GetName() const override;
    void Setup(RenderGraphBuilder& builder) override;
    void Execute(const RenderGraphContext& context) const override;

private:
    Inputs m_inputs;
    TemporalResolveConstants m_constants;
    CD3DX12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    const TemporalResolvePipeline* m_pipeline;
};

DELTA_ENGINE_NS_END
