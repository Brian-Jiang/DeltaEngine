#pragma once

#include <d3d12.h>
#include <string>
#include <vector>

namespace DeltaEngine
{
class Device;
}

namespace DeltaEngine::Tests
{

struct GpuValidationMessage
{
    D3D12_MESSAGE_SEVERITY severity = D3D12_MESSAGE_SEVERITY_INFO;
    D3D12_MESSAGE_ID id = D3D12_MESSAGE_ID(0);
    std::string text;
};

bool IsD3D12DebugLayerAvailable();
bool IsWarpAdapterAvailable();
void EnableDebugLayerForTests();
void DisableBreakOnSeverity(ID3D12Device* device);
std::vector<GpuValidationMessage> DrainD3D12InfoQueue(ID3D12Device* device);
void AssertGpuValidationClean(ID3D12Device* device);
void AssertGpuTeardownClean(Device& device);

#define EXPECT_GPU_VALIDATION_CLEAN(device) \
    ::DeltaEngine::Tests::AssertGpuValidationClean((device))

} // namespace DeltaEngine::Tests
