#include "Runtime/Graphics/RenderGraph/TemporalResolveGraphPass.h"

#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"

DELTA_ENGINE_NS_BEGIN

TemporalResolveGraphPass::TemporalResolveGraphPass(const Inputs& inputs, const TemporalResolveConstants& constants,
    CD3DX12_VIEWPORT viewport, D3D12_RECT scissorRect, const TemporalResolvePipeline* pipeline)
    : m_inputs(inputs)
    , m_constants(constants)
    , m_viewport(viewport)
    , m_scissorRect(scissorRect)
    , m_pipeline(pipeline)
{
}

const char* TemporalResolveGraphPass::GetName() const
{
    return "TemporalResolve";
}

void TemporalResolveGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Read(m_inputs.color, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Read(m_inputs.historyRead, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Read(m_inputs.motionVectors, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Read(m_inputs.depth, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Write(m_inputs.historyWrite, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void TemporalResolveGraphPass::Execute(const RenderGraphContext& context) const
{
    if (!context.commandList || !m_pipeline || !m_pipeline->GetRootSignature() || !m_pipeline->GetPipelineState())
        return;

    CommandList& commandList = *context.commandList;
    commandList.SetViewport(m_viewport);
    commandList.SetScissorRect(m_scissorRect);
    commandList.GetD3D12CommandList()->OMSetRenderTargets(1, &m_inputs.historyRtv, FALSE, nullptr);
    commandList.SetGraphicsRootSignature(m_pipeline->GetRootSignature());
    commandList.SetPipelineState(m_pipeline->GetPipelineState());

    const uint32_t textureSlot = static_cast<uint32_t>(TemporalResolveRootParameter::Textures);
    commandList.SetShaderResourceView(textureSlot, 0u, m_inputs.colorSrv);
    commandList.SetShaderResourceView(textureSlot, 1u, m_inputs.historySrv);
    commandList.SetShaderResourceView(textureSlot, 2u, m_inputs.motionSrv);
    commandList.SetShaderResourceView(textureSlot, 3u, m_inputs.depthSrv);
    commandList.SetGraphics32BitConstants(static_cast<uint32_t>(TemporalResolveRootParameter::Constants), m_constants);

    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.Draw(3u, 1u, 0u, 0u);
}

DELTA_ENGINE_NS_END
