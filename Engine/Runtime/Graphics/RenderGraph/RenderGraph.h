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

class TransientTexturePool;

struct RenderGraphResourceTransition
{
    RenderGraphTextureHandle texture;
    D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_COMMON;
};

struct RenderGraphClearOp
{
    RenderGraphTextureHandle texture;
    RenderGraphClearValue value;
};

struct RenderGraphCompiledPass
{
    size_t passIndex = 0;
    std::vector<RenderGraphResourceTransition> transitions;
    std::vector<RenderGraphClearOp> clears;
};

class DELTAENGINE_API RenderGraph
{
public:
    RenderGraph() = default;
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&) = default;
    RenderGraph& operator=(RenderGraph&&) = default;

    void AddPass(std::unique_ptr<RenderGraphPass> pass);
    RenderGraphTextureHandle ImportTexture(std::string name, std::shared_ptr<DirectX12Texture> texture,
        RenderGraphTextureUsage usage);

    /// Pool used by CreateTexture; must outlive the graph. The pool owns transient
    /// textures and recycles them by fence — Reset() only drops graph references.
    void SetTransientPool(TransientTexturePool* pool) { m_transientPool = pool; }
    TransientTexturePool* GetTransientPool() const { return m_transientPool; }

    /// Acquires a frame-scoped texture from the transient pool and registers it
    /// under the same handle/lookup path as imported textures.
    RenderGraphTextureHandle CreateTexture(std::string name, const D3D12_RESOURCE_DESC& desc,
        RenderGraphTextureUsage usage);

    void Reset();
    void Compile();
    void Execute(const RenderGraphContext& context);

    size_t GetPassCount() const { return m_passes.size(); }
    const RenderGraphPass& GetPass(size_t index) const;
    size_t GetImportedTextureCount() const { return m_importedTextures.size(); }
    RenderGraphTextureHandle FindImportedTexture(std::string_view name) const;
    const RenderGraphTexture& GetImportedTexture(RenderGraphTextureHandle handle) const;

    size_t GetCompiledPassCount() const { return m_compiledPasses.size(); }
    const RenderGraphCompiledPass& GetCompiledPass(size_t index) const;

private:
    std::vector<std::unique_ptr<RenderGraphPass>> m_passes;
    std::vector<RenderGraphTexture> m_importedTextures;
    std::vector<RenderGraphCompiledPass> m_compiledPasses;
    TransientTexturePool* m_transientPool = nullptr;
};

DELTA_ENGINE_NS_END
