#pragma once

#include "EngineIncludes.h"

#include <cstdint>

DELTA_ENGINE_NS_BEGIN

struct RenderGraphTextureTag {};

template<typename ResourceTag>
struct RenderGraphResourceHandle
{
    uint32_t index = kInvalid;

    static constexpr uint32_t kInvalid = UINT32_MAX;

    bool IsValid() const { return index != kInvalid; }

    auto operator<=>(const RenderGraphResourceHandle&) const = default;
};

using RenderGraphTextureHandle = RenderGraphResourceHandle<RenderGraphTextureTag>;

DELTA_ENGINE_NS_END
