#include "Runtime/Graphics/RenderGraph/RenderGraph.h"

#include <utility>

DELTA_ENGINE_NS_BEGIN

void RenderGraph::AddPass(std::unique_ptr<RenderGraphPass> pass)
{
    DELTA_ASSERT(pass != nullptr);
    m_passes.push_back(std::move(pass));
}

RenderGraphTextureHandle RenderGraph::ImportTexture(std::string name, std::shared_ptr<DirectX12Texture> texture,
    RenderGraphTextureUsage usage)
{
    DELTA_ASSERT(FindImportedTexture(name).index == RenderGraphTextureHandle::kInvalid);

    RenderGraphTextureHandle handle;
    handle.index = static_cast<uint32_t>(m_importedTextures.size());

    RenderGraphTexture importedTexture;
    importedTexture.name = std::move(name);
    importedTexture.texture = std::move(texture);
    importedTexture.usage = usage;
    m_importedTextures.push_back(std::move(importedTexture));

    return handle;
}

void RenderGraph::Compile()
{
    DELTA_NOT_IMPLEMENTED();
}

void RenderGraph::Execute(const RenderGraphContext& context)
{
    (void)context;
    DELTA_NOT_IMPLEMENTED();
}

const RenderGraphPass& RenderGraph::GetPass(size_t index) const
{
    DELTA_ASSERT(index < m_passes.size());
    return *m_passes[index];
}

RenderGraphTextureHandle RenderGraph::FindImportedTexture(std::string_view name) const
{
    for (size_t i = 0; i < m_importedTextures.size(); ++i)
    {
        if (m_importedTextures[i].name == name)
        {
            RenderGraphTextureHandle handle;
            handle.index = static_cast<uint32_t>(i);
            return handle;
        }
    }

    return {};
}

const RenderGraphTexture& RenderGraph::GetImportedTexture(RenderGraphTextureHandle handle) const
{
    DELTA_ASSERT(handle.IsValid());
    DELTA_ASSERT(handle.index < m_importedTextures.size());
    return m_importedTextures[handle.index];
}

DELTA_ENGINE_NS_END
