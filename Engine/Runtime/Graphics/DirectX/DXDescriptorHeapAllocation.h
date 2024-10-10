#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class DXDescriptorHeapPage;

class DXDescriptorHeapAllocation {
public:
    DXDescriptorHeapAllocation();
    DXDescriptorHeapAllocation(D3D12_CPU_DESCRIPTOR_HANDLE handle, UINT32 size, UINT increaseSize, std::shared_ptr<DXDescriptorHeapPage> page);
    DXDescriptorHeapAllocation(DXDescriptorHeapAllocation&& other);
    ~DXDescriptorHeapAllocation();

    DXDescriptorHeapAllocation& operator=(DXDescriptorHeapAllocation&& other);

    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(UINT32 offset);
    UINT32 GetDescriptorCount();
    bool IsNull() const;

private:
    void Free();

    D3D12_CPU_DESCRIPTOR_HANDLE m_rootDescriptorHandle;
    UINT m_descriptorIncreaseSize;
    UINT32 m_descriptorCount;
    std::shared_ptr<DXDescriptorHeapPage> m_descriptorHeapPage;
};

DELTA_ENGINE_NS_END