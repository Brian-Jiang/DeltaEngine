#include "Runtime/Graphics/RenderGraph/TransientTexturePool.h"

#include <algorithm>
#include <utility>

DELTA_ENGINE_NS_BEGIN

TransientTexturePool::TextureKey TransientTexturePool::MakeKey(const D3D12_RESOURCE_DESC& desc)
{
    TextureKey key;
    key.dimension = desc.Dimension;
    key.width = desc.Width;
    key.height = desc.Height;
    key.depthOrArraySize = desc.DepthOrArraySize;
    key.mipLevels = desc.MipLevels;
    key.format = desc.Format;
    key.sampleCount = desc.SampleDesc.Count;
    key.sampleQuality = desc.SampleDesc.Quality;
    key.flags = desc.Flags;
    return key;
}

std::shared_ptr<DirectX12Texture> TransientTexturePool::Acquire(const D3D12_RESOURCE_DESC& desc,
    const std::string& name)
{
    const TextureKey key = MakeKey(desc);

    for (auto it = m_free.begin(); it != m_free.end(); ++it)
    {
        if (it->key == key)
        {
            Entry entry = std::move(*it);
            m_free.erase(it);
            entry.idleFrames = 0;
            entry.fenceValue = 0;
            std::shared_ptr<DirectX12Texture> texture = entry.texture;
            m_active.push_back(std::move(entry));
            return texture;
        }
    }

    DELTA_ASSERT(m_factory != nullptr);
    if (!m_factory)
    {
        return nullptr;
    }

    std::shared_ptr<DirectX12Texture> texture = m_factory(desc, name);
    if (!texture)
    {
        return nullptr;
    }

    Entry entry;
    entry.key = key;
    entry.texture = texture;
    m_active.push_back(std::move(entry));
    return texture;
}

void TransientTexturePool::BeginFrame(uint64_t completedFenceValue)
{
    // Age out free entries that have not been reacquired for several cycles.
    std::erase_if(m_free, [](Entry& entry)
    {
        return ++entry.idleFrames > kMaxIdleFrames;
    });

    // Recycle in-flight entries whose GPU work has completed.
    for (auto it = m_inFlight.begin(); it != m_inFlight.end();)
    {
        if (it->fenceValue <= completedFenceValue)
        {
            Entry entry = std::move(*it);
            it = m_inFlight.erase(it);
            entry.idleFrames = 0;
            entry.fenceValue = 0;
            m_free.push_back(std::move(entry));
        }
        else
        {
            ++it;
        }
    }
}

void TransientTexturePool::RetireFrame(uint64_t submittedFenceValue)
{
    for (Entry& entry : m_active)
    {
        entry.fenceValue = submittedFenceValue;
        m_inFlight.push_back(std::move(entry));
    }
    m_active.clear();
}

void TransientTexturePool::Clear()
{
    m_free.clear();
    m_active.clear();
    m_inFlight.clear();
}

DELTA_ENGINE_NS_END
