#include "Runtime/Graphics/TemporalHistory.h"

#include "Runtime/Graphics/RenderGraph/RenderGraph.h"

#include <d3dx12.h>

#include <string>

using namespace DeltaEngine;

void TemporalHistory::Resize(uint32_t width, uint32_t height)
{
    DELTA_ASSERT(m_factory != nullptr);

    m_width = width;
    m_height = height;
    m_readIndex = 0;
    m_valid = false;

    const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(kTemporalHistoryFormat, width, height, 1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    for (size_t i = 0; i < m_textures.size(); ++i)
        m_textures[i] = m_factory ? m_factory(desc, "Temporal History " + std::to_string(i)) : nullptr;
}

TemporalHistoryHandles TemporalHistory::Import(RenderGraph& graph) const
{
    if (!HasTextures())
        return {};

    const RenderGraphTextureUsage usage =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    TemporalHistoryHandles handles;
    handles.read = graph.ImportTexture("TemporalHistoryRead", GetReadTexture(), usage);
    handles.write = graph.ImportTexture("TemporalHistoryWrite", GetWriteTexture(), usage);
    return handles;
}

void TemporalHistory::EndFrame(bool historyWritten)
{
    m_readIndex = 1u - m_readIndex;
    m_valid = historyWritten;
}

void TemporalHistory::Clear()
{
    m_textures = {};
    m_readIndex = 0;
    m_valid = false;
}
