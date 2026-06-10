#include "Runtime/Graphics/RenderGraph/PostProcessRenderGraphPass.h"

#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/PostProcess/PostProcessPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"

DELTA_ENGINE_NS_BEGIN

PostProcessRenderGraphPass::PostProcessRenderGraphPass(PostProcessPass* pass,
    RenderGraphTextureHandle input,
    RenderGraphTextureHandle output,
    D3D12_CPU_DESCRIPTOR_HANDLE inputSRV,
    D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
    UINT width,
    UINT height)
    : m_pass(pass)
    , m_input(input)
    , m_output(output)
    , m_inputSRV(inputSRV)
    , m_outputRTV(outputRTV)
    , m_width(width)
    , m_height(height)
{
}

const char* PostProcessRenderGraphPass::GetName() const
{
    return m_pass && !m_pass->m_passName.empty() ? m_pass->m_passName.c_str() : "PostProcess";
}

void PostProcessRenderGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Read(m_input, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Write(m_output, D3D12_RESOURCE_STATE_RENDER_TARGET,
        RenderGraphClearValue::Color4(0.0f, 0.0f, 0.0f, 1.0f));
}

void PostProcessRenderGraphPass::Execute(const RenderGraphContext& context) const
{
    if (!m_pass || !context.commandList || !context.graphicsContext)
    {
        return;
    }

    CommandList& commandList = *context.commandList;
    commandList.GetD3D12CommandList()->OMSetRenderTargets(1, &m_outputRTV, FALSE, nullptr);

    m_pass->Execute(*context.graphicsContext, m_inputSRV, m_outputRTV, m_width, m_height);
}

DELTA_ENGINE_NS_END
