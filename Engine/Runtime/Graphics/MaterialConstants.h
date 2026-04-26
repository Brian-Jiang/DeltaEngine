#pragma once

#include <cstdint>
#include <DirectXMath.h>

enum class MaterialFlags : uint32_t
{
    None                    = 0,
    HasAlbedoMap            = 1u << 0,
    HasNormalMap            = 1u << 1,
    HasMetallicRoughnessMap = 1u << 2,
    HasOcclusionMap         = 1u << 3,
    HasEmissiveMap          = 1u << 4,
    AlphaBlend              = 1u << 5,
    AlphaTest               = 1u << 6,
    DoubleSided             = 1u << 7,
    HasAlphaMask            = 1u << 8,
};

inline MaterialFlags operator|(MaterialFlags a, MaterialFlags b)
{
    return static_cast<MaterialFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline MaterialFlags operator&(MaterialFlags a, MaterialFlags b)
{
    return static_cast<MaterialFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline MaterialFlags& operator|=(MaterialFlags& a, MaterialFlags b)
{
    a = a | b;
    return a;
}

inline bool HasAny(MaterialFlags value, MaterialFlags bit)
{
    return (static_cast<uint32_t>(value) & static_cast<uint32_t>(bit)) != 0;
}

enum class MaterialTextureSlot : uint32_t
{
    Albedo            = 0,
    Normal            = 1,
    MetallicRoughness = 2,
    AO                = 3,
    Emissive          = 4,
    AlphaMask         = 5,
    Count             = 6,
};

struct alignas(256) MaterialCB
{
    DirectX::XMFLOAT4 baseColor;
    float             metallic;
    float             roughness;
    float             emissiveIntensity;
    float             alphaCutoff;
    DirectX::XMFLOAT4 emissiveColor;
    uint32_t          flags;
    uint32_t          _pad[3];
};
