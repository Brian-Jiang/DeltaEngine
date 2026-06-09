#include "Runtime/Graphics/RenderGraph/ShadowRenderGraphPass.h"

#include "Runtime/Core/DWorld.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"
#include "Runtime/Graphics/Shadow/ShadowPassManager.h"

DELTA_ENGINE_NS_BEGIN

ShadowRenderGraphPass::ShadowRenderGraphPass(ShadowPassManager* shadowPass,
    DWorld* world,
    std::shared_ptr<DXGraphicsContext> graphicsContext,
    RenderGraphTextureHandle directionalAtlas,
    RenderGraphTextureHandle spotAtlas,
    RenderGraphTextureHandle pointCubeArray)
    : m_shadowPass(shadowPass)
    , m_world(world)
    , m_graphicsContext(std::move(graphicsContext))
    , m_directionalAtlas(directionalAtlas)
    , m_spotAtlas(spotAtlas)
    , m_pointCubeArray(pointCubeArray)
{
}

const char* ShadowRenderGraphPass::GetName() const
{
    return "Shadow";
}

void ShadowRenderGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Write(m_directionalAtlas, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    builder.Write(m_spotAtlas, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    builder.Write(m_pointCubeArray, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void ShadowRenderGraphPass::Execute(const RenderGraphContext&) const
{
    if (!m_shadowPass || !m_world || !m_graphicsContext)
    {
        return;
    }

    m_shadowPass->Render(m_graphicsContext, *m_world);
}

DELTA_ENGINE_NS_END
