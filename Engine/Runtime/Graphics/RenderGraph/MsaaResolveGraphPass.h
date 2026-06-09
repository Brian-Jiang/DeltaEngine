#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"

DELTA_ENGINE_NS_BEGIN

class DirectX12Texture;

class DELTAENGINE_API MsaaResolveGraphPass final : public RenderGraphPass
{
public:
    MsaaResolveGraphPass(RenderGraphTextureHandle msaaSource,
        RenderGraphTextureHandle resolved,
        std::shared_ptr<DirectX12Texture> msaaTexture,
        std::shared_ptr<DirectX12Texture> resolvedTexture);

    const char* GetName() const override;
    void Setup(RenderGraphBuilder& builder) override;
    void Execute(const RenderGraphContext& context) const override;

private:
    RenderGraphTextureHandle m_msaaSource;
    RenderGraphTextureHandle m_resolved;
    std::shared_ptr<DirectX12Texture> m_msaaTexture;
    std::shared_ptr<DirectX12Texture> m_resolvedTexture;
};

DELTA_ENGINE_NS_END
