#pragma once

#include <memory>

namespace DeltaEngine
{
class CommandQueue;
class Device;
class DirectX12Texture;
}

namespace DeltaEngine::Tests
{

bool TextureHasNonClearContent(Device& device,
    CommandQueue& queue,
    const std::shared_ptr<DirectX12Texture>& texture,
    float clearR,
    float clearG,
    float clearB,
    float epsilon);

} // namespace DeltaEngine::Tests
