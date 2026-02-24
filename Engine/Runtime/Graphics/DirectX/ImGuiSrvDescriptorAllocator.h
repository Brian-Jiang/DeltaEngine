#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <wrl.h>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class Device;

/**
 * Free-list allocator for shader-visible SRV descriptors.
 * Used by ImGui DX12 backend for font texture and user textures.
 */
class ImGuiSrvDescriptorAllocator
{
public:
    DELTAENGINE_API ImGuiSrvDescriptorAllocator() = default;

    /**
     * Initialize with a device and shader-visible CBV_SRV_UAV heap.
     * The heap must be created via Device::CreateShaderVisibleSrvHeap().
     */
    DELTAENGINE_API void Create(Device& device, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap);

    DELTAENGINE_API void Destroy();

    DELTAENGINE_API void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle);
    DELTAENGINE_API void Free(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

    ID3D12DescriptorHeap* GetHeap() const { return m_heap.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_heapStartCpu = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_heapStartGpu = {};
    UINT m_heapHandleIncrement = 0;
    std::vector<int> m_freeIndices;
};

DELTA_ENGINE_NS_END
