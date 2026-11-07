#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"
#include "Runtime/Graphics/RenderGraph/TransientTexturePool.h"

#include <d3d12.h>

#include <array>
#include <cstdint>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class DirectX12Texture;
class RenderGraph;

/** Scene-colour format used by the temporal history textures. */
inline constexpr DXGI_FORMAT kTemporalHistoryFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

struct TemporalHistoryHandles
{
    /** Previous frame's accumulated colour. */
    RenderGraphTextureHandle read;
    /** Target for this frame's accumulated colour. */
    RenderGraphTextureHandle write;
};

/// Two persistent single-sample history textures whose read/write roles swap every frame.
/// Owned outside the transient pool so they survive across frames.
class TemporalHistory
{
public:
    using TextureFactory = TransientTexturePool::TextureFactory;

    void SetTextureFactory(TextureFactory factory) { m_factory = std::move(factory); }

    /** Recreates both textures and discards history. */
    DELTAENGINE_API void Resize(uint32_t width, uint32_t height);

    void Invalidate() { m_valid = false; }

    /** Returns invalid handles when no textures exist. */
    DELTAENGINE_API TemporalHistoryHandles Import(RenderGraph& graph) const;

    /** Swaps read/write roles; history is valid next frame only if this frame wrote it. */
    DELTAENGINE_API void EndFrame(bool historyWritten);

    DELTAENGINE_API void Clear();

    /** True when the read texture holds usable history. */
    bool IsValid() const { return m_valid; }
    bool HasTextures() const { return m_textures[0] && m_textures[1]; }
    uint32_t GetReadIndex() const { return m_readIndex; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    const std::shared_ptr<DirectX12Texture>& GetReadTexture() const { return m_textures[m_readIndex]; }
    const std::shared_ptr<DirectX12Texture>& GetWriteTexture() const { return m_textures[1u - m_readIndex]; }

private:
    TextureFactory m_factory;
    std::array<std::shared_ptr<DirectX12Texture>, 2> m_textures;
    uint32_t m_readIndex = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_valid = false;
};

DELTA_ENGINE_NS_END
