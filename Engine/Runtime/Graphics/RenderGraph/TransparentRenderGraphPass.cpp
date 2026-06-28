#include "Runtime/Graphics/RenderGraph/TransparentRenderGraphPass.h"

#include <utility>

#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"

DELTA_ENGINE_NS_BEGIN

TransparentRenderGraphPass::TransparentRenderGraphPass(RenderGraphTextureHandle color,
    RenderGraphTextureHandle depth,
    RenderTarget* renderTarget,
    std::shared_ptr<RootSignature> rootSignature,
    CD3DX12_VIEWPORT viewport,
    D3D12_RECT scissorRect,
    DescriptorStageCallback stageDescriptors,
    std::shared_ptr<DXGraphicsContext> graphicsContext,
    SceneDrawCallback drawCallback)
    : m_color(color)
    , m_depth(depth)
    , m_renderTarget(renderTarget)
    , m_rootSignature(std::move(rootSignature))
    , m_viewport(viewport)
    , m_scissorRect(scissorRect)
    , m_stageDescriptors(std::move(stageDescriptors))
    , m_graphicsContext(std::move(graphicsContext))
    , m_drawCallback(std::move(drawCallback))
{
}

const char* TransparentRenderGraphPass::GetName() const
{
    return "Transparent";
}

void TransparentRenderGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Write(m_color, D3D12_RESOURCE_STATE_RENDER_TARGET);
    builder.Write(m_depth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void TransparentRenderGraphPass::Execute(const RenderGraphContext& context) const
{
    if (!context.commandList || !m_renderTarget || !m_graphicsContext)
    {
        return;
    }

    CommandList& commandList = *context.commandList;
    commandList.SetViewport(m_viewport);
    commandList.SetScissorRect(m_scissorRect);
    commandList.BindRenderTarget(*m_renderTarget);
    if (m_rootSignature)
    {
        commandList.SetGraphicsRootSignature(m_rootSignature);
    }

    if (m_stageDescriptors)
    {
        m_stageDescriptors(commandList);
    }

    const ScenePassType previousPass = m_graphicsContext->activePass;
    m_graphicsContext->activePass = ScenePassType::Transparent;

    if (m_drawCallback)
    {
        m_drawCallback(m_graphicsContext);
    }

    m_graphicsContext->activePass = previousPass;
}

DELTA_ENGINE_NS_END
