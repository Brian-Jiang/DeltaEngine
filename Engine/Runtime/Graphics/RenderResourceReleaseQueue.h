#pragma once

#include "EngineIncludes.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class RenderProxy;

struct RenderResourceReleaseToken
{
    uint64_t m_id = 0;

    bool IsValid() const { return m_id != 0; }

    static RenderResourceReleaseToken Invalid() { return {}; }

    friend bool operator==(const RenderResourceReleaseToken&, const RenderResourceReleaseToken&) = default;
};

/// Fence-gated queue that drops render-proxy GPU resources once the GPU has finished
/// using them. Never blocks the calling thread.
class RenderResourceReleaseQueue
{
public:
    using FenceCompleteFn = std::function<bool(uint64_t fenceValue)>;

    DELTAENGINE_API void SetFenceCompleteChecker(FenceCompleteFn checker);

    /// Fence value captured at the last RenderFrame submit; used when enqueue omits an explicit fence.
    DELTAENGINE_API void SetLastSubmittedFence(uint64_t fenceValue);

    DELTAENGINE_API RenderResourceReleaseToken Enqueue(std::shared_ptr<RenderProxy> proxy);
    DELTAENGINE_API RenderResourceReleaseToken Enqueue(std::shared_ptr<RenderProxy> proxy, uint64_t fenceValue);

    DELTAENGINE_API bool IsComplete(RenderResourceReleaseToken token) const;

    /// Drops entries whose fence has completed. Returns the number of entries released.
    DELTAENGINE_API size_t ProcessCompleted();

    DELTAENGINE_API size_t GetPendingCount() const;

private:
    struct Entry
    {
        RenderResourceReleaseToken     token;
        uint64_t                       fenceValue = 0;
        std::shared_ptr<RenderProxy>   proxy;
    };

    FenceCompleteFn           m_fenceCompleteChecker;
    uint64_t                  m_lastSubmittedFence = 0;
    uint64_t                  m_nextTokenId        = 1;
    std::vector<Entry>        m_pending;
};

DELTA_ENGINE_NS_END
