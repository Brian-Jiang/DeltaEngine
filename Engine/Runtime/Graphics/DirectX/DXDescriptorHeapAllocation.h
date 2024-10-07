#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class DXDescriptorHeapPage;

class DXDescriptorHeapAllocation {
public:
    DXDescriptorHeapAllocation();
    DXDescriptorHeapAllocation(DXDescriptorHeapAllocation&& other);

    DXDescriptorHeapAllocation& operator=(DXDescriptorHeapAllocation&& other);

    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(UINT32 offset);

private:
    void Free();

    D3D12_CPU_DESCRIPTOR_HANDLE m_rootDescriptorHandle;
    UINT m_descriptorIncreaseSize;
    UINT32 m_descriptorCount;
    std::shared_ptr<DXDescriptorHeapPage> m_descriptorHeapPage;
};

DELTA_ENGINE_NS_END