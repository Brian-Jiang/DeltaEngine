#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>

#include <cstddef>
#include <cstdint>

DELTA_ENGINE_NS_BEGIN

/// Object constant buffer layout (must match StandardConstantStructs.slang Object, b1).
struct alignas(16) ObjectCB
{
    /// World matrix, transposed.
    DirectX::XMMATRIX worldMatrix;

    /// Previous frame world matrix, transposed.
    DirectX::XMMATRIX prevWorldMatrix;

    DirectX::XMFLOAT4 color{ 1.f, 1.f, 1.f, 1.f };
    uint32_t useInstanceMatrix = 0;
    uint32_t pad[3]{};
};

static_assert(sizeof(ObjectCB) % 16 == 0);
static_assert(offsetof(ObjectCB, prevWorldMatrix) == 64);
static_assert(offsetof(ObjectCB, color) == 128);
static_assert(offsetof(ObjectCB, useInstanceMatrix) == 144);

DELTA_ENGINE_NS_END
