#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <memory>
#include <vector>
#include <set>
#include <mutex>

#include "Graphics/DirectX/DescriptorAllocatorPage.h"
#include "Graphics/DirectX/DescriptorAllocation.h"

DELTA_ENGINE_NS_BEGIN

class DescriptorAllocatorPage;
class Device;

class DescriptorAllocator
{
public:
    /**
     * Allocate a number of contiguous descriptors from a CPU visible descriptor heap.
     *
     * @param numDescriptors The number of contiguous descriptors to allocate.
     * Cannot be more than the number of descriptors per descriptor heap.
     */
    DescriptorAllocation Allocate(uint32_t numDescriptors = 1);

    /**
     * When the frame has completed, the stale descriptors can be released.
     */
    void ReleaseStaleDescriptors();

//protected:
    friend struct std::default_delete<DescriptorAllocator>;
    friend class Device;
    friend class DXRenderManager;

    // Can only be created by the Device.
    DescriptorAllocator(Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256);
    virtual ~DescriptorAllocator();

private:
    using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

    // Create a new heap with a specific number of descriptors.
    std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

    // The device that was use to create this DescriptorAllocator.
    Device& m_Device;
    D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
    uint32_t                   m_NumDescriptorsPerHeap;

    DescriptorHeapPool m_HeapPool;
    // Indices of available heaps in the heap pool.
    std::set<size_t> m_AvailableHeaps;

    std::mutex m_AllocationMutex;
};

DELTA_ENGINE_NS_END