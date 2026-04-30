#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>
#include <cstdint>

DELTA_ENGINE_NS_BEGIN

enum class LightType : uint32_t
{
    Directional,
    Spot,
    Point,
};

struct ShadowAtlasSlot
{
    uint32_t atlasIndex = 0;
    uint32_t tileIndex = 0;
};

struct ShadowAllocation
{
    DirectX::XMFLOAT4 atlasUVRect = { 0.0f, 0.0f, 1.0f, 1.0f };
    int32_t cubeArrayIndex = -1;
    uint32_t _pad = 0;
};

struct ShadowView
{
    DirectX::XMMATRIX viewProj = DirectX::XMMatrixIdentity();
    ShadowAtlasSlot slot{};
    LightType type = LightType::Directional;
    uint32_t lightIndex = 0;
    uint32_t cubeFace = 0;
};

DELTA_ENGINE_NS_END
