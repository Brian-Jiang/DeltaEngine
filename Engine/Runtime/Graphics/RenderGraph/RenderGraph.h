#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphTexture.h"

DELTA_ENGINE_NS_BEGIN

class DELTAENGINE_API RenderGraph
{
public:
    void AddPass(std::unique_ptr<RenderGraphPass> pass);
    RenderGraphTextureHandle ImportTexture(std::string name, std::shared_ptr<DirectX12Texture> texture,
        RenderGraphTextureUsage usage);

    void Compile();
    void Execute(const RenderGraphContext& context);

    size_t GetPassCount() const { return m_passes.size(); }
    const RenderGraphPass& GetPass(size_t index) const;
    size_t GetImportedTextureCount() const { return m_importedTextures.size(); }
    RenderGraphTextureHandle FindImportedTexture(std::string_view name) const;
    const RenderGraphTexture& GetImportedTexture(RenderGraphTextureHandle handle) const;

private:
    std::vector<std::unique_ptr<RenderGraphPass>> m_passes;
    std::vector<RenderGraphTexture> m_importedTextures;
};

DELTA_ENGINE_NS_END
