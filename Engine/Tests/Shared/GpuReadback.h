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

struct GpuReadbackPixel
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 0.0f;
};

bool ReadTextureCenterPixel(Device& device,
    CommandQueue& queue,
    const std::shared_ptr<DirectX12Texture>& texture,
    GpuReadbackPixel& outPixel);

bool TextureHasNonClearContent(Device& device,
    CommandQueue& queue,
    const std::shared_ptr<DirectX12Texture>& texture,
    float clearR,
    float clearG,
    float clearB,
    float epsilon);

} // namespace DeltaEngine::Tests
