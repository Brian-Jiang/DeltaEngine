#include "Runtime/Graphics/RenderGraph/PostProcessFinalizeGraphPass.h"

#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"

DELTA_ENGINE_NS_BEGIN

PostProcessFinalizeGraphPass::PostProcessFinalizeGraphPass(RenderGraphTextureHandle output)
    : m_output(output)
{
}

const char* PostProcessFinalizeGraphPass::GetName() const
{
    return "PostProcessFinalize";
}

void PostProcessFinalizeGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Read(m_output, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void PostProcessFinalizeGraphPass::Execute(const RenderGraphContext&) const
{
}

DELTA_ENGINE_NS_END
