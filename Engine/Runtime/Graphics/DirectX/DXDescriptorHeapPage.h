#pragma once

#include "EngineIncludes.h"

#include <wrl.h>
#include <d3d12.h>
#include <map>
#include <memory>
#include <queue>

DELTA_ENGINE_NS_BEGIN

class DXDescriptorHeapAllocation;

class DXDescriptorHeapPage : public std::enable_shared_from_this<DXDescriptorHeapPage>
{
public:
    DXDescriptorHeapPage(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT32 totalDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType);

    DXDescriptorHeapAllocation Allocate(UINT32 descriptorCount);
    void Release(DXDescriptorHeapAllocation&& allocation, UINT64 frameNum);
    void ReleaseAllStale(UINT64 frameNum);

    UINT32 GetFreeDescriptorCount() const { return m_freeDescriptorCount; }

private:
    void AddFreeBlock(UINT32 offset, UINT32 descriptorCount);
    void FreeSingleStale(UINT32 offset, UINT32 descriptorCount);
    UINT32 CalculateOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;

    UINT32 m_totalDescriptorCount;
    UINT32 m_freeDescriptorCount;
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

    struct StaleDescriptorsInfo {
        StaleDescriptorsInfo(UINT32 offset, UINT32 count, UINT64 frame);

        UINT32 m_offset;
        UINT32 m_count;
        UINT64 m_frameNum;
    };

    std::queue<StaleDescriptorsInfo> m_staleDescriptorsQueue;
};

DELTA_ENGINE_NS_END