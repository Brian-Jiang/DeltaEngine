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

/// Optional clear request attached to a graph write; executed by the graph
/// after the resource has been transitioned to its declared write state.
struct RenderGraphClearValue
{
    enum class Type
    {
        None,
        Color,
        DepthStencil,
    };

    Type type = Type::None;
    float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    float depth = 1.0f;
    uint8_t stencil = 0;

    static RenderGraphClearValue Color4(float r, float g, float b, float a)
    {
        RenderGraphClearValue value;
        value.type = Type::Color;
        value.color[0] = r;
        value.color[1] = g;
        value.color[2] = b;
        value.color[3] = a;
        return value;
    }

    static RenderGraphClearValue DepthStencil(float depth, uint8_t stencil = 0)
    {
        RenderGraphClearValue value;
        value.type = Type::DepthStencil;
        value.depth = depth;
        value.stencil = stencil;
        return value;
    }
};

DELTA_ENGINE_NS_END
