#include "Runtime/Graphics/RenderGraph/TemporalSharpenGraphPass.h"

#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"

DELTA_ENGINE_NS_BEGIN

TemporalSharpenGraphPass::TemporalSharpenGraphPass(const Inputs& inputs, const TemporalSharpenConstants& constants,
    CD3DX12_VIEWPORT viewport, D3D12_RECT scissorRect, const TemporalResolvePipeline* pipeline)
    : m_inputs(inputs)
    , m_constants(constants)
    , m_viewport(viewport)
    , m_scissorRect(scissorRect)
    , m_pipeline(pipeline)
{
}

const char* TemporalSharpenGraphPass::GetName() const
{
    return "TemporalSharpen";
}

void TemporalSharpenGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Read(m_inputs.input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Write(m_inputs.output, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void TemporalSharpenGraphPass::Execute(const RenderGraphContext& context) const
{
    if (!context.commandList || !m_pipeline || !m_pipeline->GetRootSignature()
        || !m_pipeline->GetSharpenPipelineState())
        return;

    CommandList& commandList = *context.commandList;
    commandList.SetViewport(m_viewport);
    commandList.SetScissorRect(m_scissorRect);
    commandList.GetD3D12CommandList()->OMSetRenderTargets(1, &m_inputs.outputRtv, FALSE, nullptr);
    commandList.SetGraphicsRootSignature(m_pipeline->GetRootSignature());
    commandList.SetPipelineState(m_pipeline->GetSharpenPipelineState());

    // The shared table spans t0-t3; fill every slot so no stale descriptor is staged.
    const uint32_t textureSlot = static_cast<uint32_t>(TemporalResolveRootParameter::Textures);
    for (uint32_t i = 0; i < 4u; ++i)
        commandList.SetShaderResourceView(textureSlot, i, m_inputs.inputSrv);
    commandList.SetGraphics32BitConstants(static_cast<uint32_t>(TemporalResolveRootParameter::Constants), m_constants);

    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.Draw(3u, 1u, 0u, 0u);
}

DELTA_ENGINE_NS_END
