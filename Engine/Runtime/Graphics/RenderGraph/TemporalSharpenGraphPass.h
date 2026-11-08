#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"
#include "Runtime/Graphics/TemporalResolvePipeline.h"

#include <d3d12.h>
#include <d3dx12.h>

DELTA_ENGINE_NS_BEGIN

/// Contrast-clamped unsharp mask applied to the TAA output (TAASharpen.slang).
class DELTAENGINE_API TemporalSharpenGraphPass final : public RenderGraphPass
{
public:
    struct Inputs
    {
        RenderGraphTextureHandle input;
        RenderGraphTextureHandle output;
        D3D12_CPU_DESCRIPTOR_HANDLE inputSrv{};
        D3D12_CPU_DESCRIPTOR_HANDLE outputRtv{};
    };

    TemporalSharpenGraphPass(const Inputs& inputs, const TemporalSharpenConstants& constants,
        CD3DX12_VIEWPORT viewport, D3D12_RECT scissorRect, const TemporalResolvePipeline* pipeline);

    const char* GetName() const override;
    void Setup(RenderGraphBuilder& builder) override;
    void Execute(const RenderGraphContext& context) const override;

private:
    Inputs m_inputs;
    TemporalSharpenConstants m_constants;
    CD3DX12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    const TemporalResolvePipeline* m_pipeline;
};

DELTA_ENGINE_NS_END
