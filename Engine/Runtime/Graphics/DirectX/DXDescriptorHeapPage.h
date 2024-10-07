#pragma once

#include "EngineIncludes.h"

#include <wrl.h>
#include <d3d12.h>
#include <map>

DELTA_ENGINE_NS_BEGIN

class DXDescriptorHeapAllocation;

class DXDescriptorHeapPage {
public:
    DXDescriptorHeapPage(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT32 totalDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType);

    DXDescriptorHeapAllocation Allocate(UINT32 descriptorCount);

private:
    void AddFreeBlock(UINT32 offset, UINT32 descriptorCount);

    UINT32 m_totalDescriptorCount;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dxHeap;
    UINT m_descriptorIncreaseSize;
    D3D12_CPU_DESCRIPTOR_HANDLE m_rootDescriptorHandle;
    D3D12_DESCRIPTOR_HEAP_TYPE m_heapType;
    
    struct FreeBlockEntry;
    using OffsetToFreeBlockEntry = std::map<UINT32, FreeBlockEntry>;
    OffsetToFreeBlockEntry m_offsetToFreeBlock;
    using SizeToFreeBlockEntries = std::multimap<UINT32, OffsetToFreeBlockEntry::iterator>;
    SizeToFreeBlockEntries m_sizeToFreeBlocks;
    struct FreeBlockEntry {
        SizeToFreeBlockEntries::iterator m_sizeToFreeBlocksIt;
    };
};

DELTA_ENGINE_NS_END