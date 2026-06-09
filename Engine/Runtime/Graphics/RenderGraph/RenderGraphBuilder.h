#pragma once

#include "EngineIncludes.h"

#include <cstddef>
#include <vector>

#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"

DELTA_ENGINE_NS_BEGIN

class RenderGraph;

enum class RenderGraphAccessType
{
    Read,
    Write,
};

struct RenderGraphResourceAccess
{
    RenderGraphTextureHandle texture;
    D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
    RenderGraphAccessType type = RenderGraphAccessType::Read;
};

class DELTAENGINE_API RenderGraphBuilder
{
public:
    explicit RenderGraphBuilder(RenderGraph& graph) : m_graph(graph) {}

    RenderGraph& GetGraph() { return m_graph; }
    const RenderGraph& GetGraph() const { return m_graph; }

    void Read(RenderGraphTextureHandle texture, D3D12_RESOURCE_STATES state);
    void Write(RenderGraphTextureHandle texture, D3D12_RESOURCE_STATES state);

    void BeginPass(size_t passIndex);

    const std::vector<std::vector<RenderGraphResourceAccess>>& GetPassAccesses() const { return m_passAccesses; }

private:
    RenderGraph& m_graph;
    std::vector<std::vector<RenderGraphResourceAccess>> m_passAccesses;
    size_t m_currentPass = 0;
};

DELTA_ENGINE_NS_END
