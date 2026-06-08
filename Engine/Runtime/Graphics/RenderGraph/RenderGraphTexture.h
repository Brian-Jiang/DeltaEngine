#pragma once

#include "EngineIncludes.h"

#include <cstdint>
#include <memory>
#include <string>

#include "Runtime/Graphics/DirectX/DirectX12Texture.h"

DELTA_ENGINE_NS_BEGIN

enum class RenderGraphTextureUsage : uint32_t
{
    None = 0,
    ColorAttachment = 1u << 0,
    DepthAttachment = 1u << 1,
    ShaderResource = 1u << 2,
    CopySrc = 1u << 3,
    CopyDst = 1u << 4,
};

inline RenderGraphTextureUsage operator|(RenderGraphTextureUsage a, RenderGraphTextureUsage b)
{
    return static_cast<RenderGraphTextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline RenderGraphTextureUsage operator&(RenderGraphTextureUsage a, RenderGraphTextureUsage b)
{
    return static_cast<RenderGraphTextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline RenderGraphTextureUsage& operator|=(RenderGraphTextureUsage& a, RenderGraphTextureUsage b)
{
    a = a | b;
    return a;
}

inline bool HasAny(RenderGraphTextureUsage value, RenderGraphTextureUsage bit)
{
    return (static_cast<uint32_t>(value) & static_cast<uint32_t>(bit)) != 0;
}

struct RenderGraphTexture
{
    std::string name;
    std::shared_ptr<DirectX12Texture> texture;
    RenderGraphTextureUsage usage = RenderGraphTextureUsage::None;
};

DELTA_ENGINE_NS_END
