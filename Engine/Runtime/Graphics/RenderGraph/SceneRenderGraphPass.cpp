#include "Runtime/Graphics/RenderGraph/SceneRenderGraphPass.h"

#include <utility>

#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"

DELTA_ENGINE_NS_BEGIN

SceneRenderGraphPass::SceneRenderGraphPass(RenderGraphTextureHandle color,
    RenderGraphTextureHandle depth,
    RenderGraphClearValue colorClear,
    RenderGraphClearValue depthClear,
    RenderTarget* renderTarget,
    std::shared_ptr<RootSignature> rootSignature,
    CD3DX12_VIEWPORT viewport,
    D3D12_RECT scissorRect,
    DescriptorStageCallback stageDescriptors,
    std::shared_ptr<DXGraphicsContext> graphicsContext,
    SceneDrawCallback drawCallback)
    : m_color(color)
    , m_depth(depth)
    , m_colorClear(colorClear)
    , m_depthClear(depthClear)
    , m_renderTarget(renderTarget)
    , m_rootSignature(std::move(rootSignature))
    , m_viewport(viewport)
    , m_scissorRect(scissorRect)
    , m_stageDescriptors(std::move(stageDescriptors))
    , m_graphicsContext(std::move(graphicsContext))
    , m_drawCallback(std::move(drawCallback))
{
}

const char* SceneRenderGraphPass::GetName() const
{
    return "Scene";
}

void SceneRenderGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Write(m_color, D3D12_RESOURCE_STATE_RENDER_TARGET, m_colorClear);
    builder.Write(m_depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, m_depthClear);
}

void SceneRenderGraphPass::Execute(const RenderGraphContext& context) const
{
    if (!context.commandList || !m_renderTarget)
    {
        return;
    }

    CommandList& commandList = *context.commandList;
    commandList.SetViewport(m_viewport);
    commandList.SetScissorRect(m_scissorRect);
    commandList.SetRenderTarget(*m_renderTarget);
    if (m_rootSignature)
    {
        commandList.SetGraphicsRootSignature(m_rootSignature);
    }

    if (m_stageDescriptors)
    {
        m_stageDescriptors(commandList);
    }

    if (m_drawCallback)
    {
        m_drawCallback(m_graphicsContext);
    }
}

DELTA_ENGINE_NS_END
