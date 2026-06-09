#include "Runtime/Graphics/RenderGraph/MsaaResolveGraphPass.h"

#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"

DELTA_ENGINE_NS_BEGIN

MsaaResolveGraphPass::MsaaResolveGraphPass(RenderGraphTextureHandle msaaSource,
    RenderGraphTextureHandle resolved,
    std::shared_ptr<DirectX12Texture> msaaTexture,
    std::shared_ptr<DirectX12Texture> resolvedTexture)
    : m_msaaSource(msaaSource)
    , m_resolved(resolved)
    , m_msaaTexture(std::move(msaaTexture))
    , m_resolvedTexture(std::move(resolvedTexture))
{
}

const char* MsaaResolveGraphPass::GetName() const
{
    return "MsaaResolve";
}

void MsaaResolveGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Read(m_msaaSource, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
    builder.Write(m_resolved, D3D12_RESOURCE_STATE_RESOLVE_DEST);
}

void MsaaResolveGraphPass::Execute(const RenderGraphContext& context) const
{
    if (!context.commandList || !m_msaaTexture || !m_resolvedTexture)
    {
        return;
    }

    context.commandList->ResolveSubresourceNoBarrier(m_resolvedTexture, m_msaaTexture);
}

DELTA_ENGINE_NS_END
