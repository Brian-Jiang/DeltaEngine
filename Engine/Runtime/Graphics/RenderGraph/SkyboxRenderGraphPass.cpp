#include "Runtime/Graphics/RenderGraph/SkyboxRenderGraphPass.h"

#include <utility>

#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/Skybox.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"

DELTA_ENGINE_NS_BEGIN

SkyboxRenderGraphPass::SkyboxRenderGraphPass(RenderGraphTextureHandle color,
    RenderGraphTextureHandle depth,
    RenderTarget* renderTarget,
    std::shared_ptr<RootSignature> rootSignature,
    CD3DX12_VIEWPORT viewport,
    D3D12_RECT scissorRect,
    DescriptorStageCallback stageDescriptors,
    DWorld* world,
    std::shared_ptr<DXGraphicsContext> graphicsContext)
    : m_color(color)
    , m_depth(depth)
    , m_renderTarget(renderTarget)
    , m_rootSignature(std::move(rootSignature))
    , m_viewport(viewport)
    , m_scissorRect(scissorRect)
    , m_stageDescriptors(std::move(stageDescriptors))
    , m_world(world)
    , m_graphicsContext(std::move(graphicsContext))
{
}

const char* SkyboxRenderGraphPass::GetName() const
{
    return "Skybox";
}

void SkyboxRenderGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Write(m_color, D3D12_RESOURCE_STATE_RENDER_TARGET);
    builder.Write(m_depth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void SkyboxRenderGraphPass::Execute(const RenderGraphContext& context) const
{
    if (!context.commandList || !m_renderTarget || !m_world)
    {
        return;
    }

    Skybox* skybox = m_world->GetSkybox();
    if (!skybox)
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

    skybox->GatherDrawCalls(m_graphicsContext);
}

DELTA_ENGINE_NS_END
