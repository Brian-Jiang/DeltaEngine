#pragma once

#include "EngineIncludes.h"

#include <cstdint>
#include <d3d12.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class DirectX12Texture;

/// Frame-scoped pool of committed transient textures (no aliasing). Textures are
/// keyed by their resource description and recycled once the GPU fence stamped at
/// RetireFrame has completed. Free entries that stay unused for kMaxIdleFrames
/// recycle cycles are destroyed, so resizes do not accumulate stale sizes.
class DELTAENGINE_API TransientTexturePool
{
public:
    /// Creates (and names) a texture for the given description. The pool itself
    /// never dereferences the returned texture, so tests can inject stubs.
    using TextureFactory =
        std::function<std::shared_ptr<DirectX12Texture>(const D3D12_RESOURCE_DESC&, const std::string& name)>;

    static constexpr uint32_t kMaxIdleFrames = 3;

    void SetTextureFactory(TextureFactory factory) { m_factory = std::move(factory); }

    /// Returns a pooled texture matching desc, creating one via the factory if no
    /// free entry matches. The texture stays owned by the pool and is recycled
    /// after the fence passed to RetireFrame completes.
    std::shared_ptr<DirectX12Texture> Acquire(const D3D12_RESOURCE_DESC& desc, const std::string& name);

    /// Recycles in-flight entries whose retire fence has completed, and destroys
    /// free entries that have stayed unused for kMaxIdleFrames cycles.
    void BeginFrame(uint64_t completedFenceValue);

    /// Moves entries acquired since the previous RetireFrame to the in-flight
    /// list, stamped with the submitted fence. May be called more than once per
    /// frame (e.g. scene submit, then editor submit).
    void RetireFrame(uint64_t submittedFenceValue);

    /// Drops every pooled texture. Call only when the GPU is idle.
    void Clear();

    size_t GetFreeCount() const { return m_free.size(); }
    size_t GetActiveCount() const { return m_active.size(); }
    size_t GetInFlightCount() const { return m_inFlight.size(); }
    size_t GetTotalCount() const { return m_free.size() + m_active.size() + m_inFlight.size(); }

private:
    struct TextureKey
    {
        D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_UNKNOWN;
        uint64_t width = 0;
        uint32_t height = 0;
        uint16_t depthOrArraySize = 0;
        uint16_t mipLevels = 0;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        uint32_t sampleCount = 0;
        uint32_t sampleQuality = 0;
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        bool operator==(const TextureKey&) const = default;
    };

    struct Entry
    {
        TextureKey key;
        std::shared_ptr<DirectX12Texture> texture;
        uint64_t fenceValue = 0;
        uint32_t idleFrames = 0;
    };

    static TextureKey MakeKey(const D3D12_RESOURCE_DESC& desc);

    TextureFactory m_factory;
    std::vector<Entry> m_free;
    std::vector<Entry> m_active;
    std::vector<Entry> m_inFlight;
};

DELTA_ENGINE_NS_END
