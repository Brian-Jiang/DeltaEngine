#pragma once

#include <cstdint>
#include <memory>
#include <vector>

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

struct GpuReadbackImage
{
    uint32_t width = 0;
    uint32_t height = 0;
    std::vector<GpuReadbackPixel> pixels;

    const GpuReadbackPixel& At(uint32_t x, uint32_t y) const { return pixels[static_cast<size_t>(y) * width + x]; }
};

/** Reads mip 0 of a single-sample 2D texture (RGBA16F, RG16F, RGBA8/BGRA8). */
bool ReadTexturePixels(Device& device,
    CommandQueue& queue,
    const std::shared_ptr<DirectX12Texture>& texture,
    GpuReadbackImage& outImage);

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
