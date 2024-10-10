#include "DXDescriptorHeapPage.h"

#include "Graphics/DXUtils.h"
#include "Graphics/DirectX/DXDescriptorHeapAllocation.h"

using namespace DeltaEngine;

DXDescriptorHeapPage::DXDescriptorHeapPage(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT32 totalDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType) 
    : m_heapType(heapType), m_totalDescriptorCount(totalDescriptorCount), m_freeDescriptorCount(totalDescriptorCount)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = totalDescriptorCount;
    heapDesc.Type = heapType;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_dxHeap)));
    m_descriptorIncreaseSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    AddFreeBlock(0, totalDescriptorCount);
}

DXDescriptorHeapAllocation DXDescriptorHeapPage::Allocate(UINT32 descriptorCount) {
    if (descriptorCount > m_freeDescriptorCount) {
        return DXDescriptorHeapAllocation();
    }

    auto sizeIt = m_sizeToFreeBlocks.lower_bound(descriptorCount);
    if (sizeIt == m_sizeToFreeBlocks.end()) {
        return DXDescriptorHeapAllocation();
    }
    
    m_freeDescriptorCount -= descriptorCount;
    auto offset = sizeIt->second->first;
    auto size = sizeIt->first;
    m_sizeToFreeBlocks.erase(sizeIt);
    m_offsetToFreeBlock.erase(sizeIt->second);
    if (size != descriptorCount) {
        AddFreeBlock(offset + descriptorCount, size - descriptorCount);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_dxHeap->GetCPUDescriptorHandleForHeapStart(), offset, m_descriptorIncreaseSize);
    DXDescriptorHeapAllocation allocation(handle, descriptorCount, m_descriptorIncreaseSize, shared_from_this());
    return allocation;
}

void DXDescriptorHeapPage::Release(DXDescriptorHeapAllocation&& allocation, UINT64 frameNum) {

    
    auto offset = CalculateOffset(allocation.GetDescriptorHandle(0));
    auto descriptorCount = allocation.GetDescriptorCount();
    m_staleDescriptorsQueue.emplace(offset, descriptorCount, frameNum);


    //m_freeDescriptorCount += descriptorCount;

    //auto previousIt = m_offsetToFreeBlock.lower_bound(offset);
    //if (previousIt != m_offsetToFreeBlock.end()) {
    //    auto& entry = previousIt->second;
    //    auto size = entry.m_sizeToFreeBlocksIt->first;
    //    if (previousIt->first + size == offset) {
    //        offset = previousIt->first;
    //        descriptorCount += size;
    //        m_sizeToFreeBlocks.erase(entry.m_sizeToFreeBlocksIt);
    //        m_offsetToFreeBlock.erase(previousIt);
    //    }
    //}

    //auto nextIt = m_offsetToFreeBlock.upper_bound(offset);
    //if (nextIt != m_offsetToFreeBlock.end()) {
    //    if (offset + descriptorCount == nextIt->first) {
    //        auto& entry = nextIt->second;
    //        auto size = entry.m_sizeToFreeBlocksIt->first;
    //        descriptorCount += size;
    //        m_sizeToFreeBlocks.erase(entry.m_sizeToFreeBlocksIt);
    //        m_offsetToFreeBlock.erase(nextIt->first);
    //    }
    //}

    //AddFreeBlock(offset, descriptorCount);
}

void DXDescriptorHeapPage::ReleaseAllStale(UINT64 frameNum) {
    while (!m_staleDescriptorsQueue.empty()) {
        auto& staleDescriptorInfo = m_staleDescriptorsQueue.front();
        if (staleDescriptorInfo.m_frameNum > frameNum) {
            break;
        }

        m_staleDescriptorsQueue.pop();
        FreeSingleStale(staleDescriptorInfo.m_offset, staleDescriptorInfo.m_count);
    }
}

void DXDescriptorHeapPage::AddFreeBlock(UINT32 offset, UINT32 descriptorCount) {
    //auto previousIt = m_offsetToFreeBlock.lower_bound(offset);
    //if (previousIt != m_offsetToFreeBlock.end()) {
    //    auto& entry = previousIt->second;
    //    auto size = entry.m_sizeToFreeBlocksIt->first;
    //    if (previousIt->first + size == offset) {
    //        offset = previousIt->first;
    //        descriptorCount += size;
    //        m_sizeToFreeBlocks.erase(entry.m_sizeToFreeBlocksIt);
    //        m_offsetToFreeBlock.erase(previousIt);
    //    }
    //}

    //auto nextIt = m_offsetToFreeBlock.upper_bound(offset);
    //if (nextIt != m_offsetToFreeBlock.end()) {
    //    if (offset + descriptorCount == nextIt->first) {
    //        auto& entry = nextIt->second;
    //        auto size = entry.m_sizeToFreeBlocksIt->first;
    //        descriptorCount += size;
    //        m_sizeToFreeBlocks.erase(entry.m_sizeToFreeBlocksIt);
    //        m_offsetToFreeBlock.erase(nextIt->first);
    //    }
    //}

    FreeBlockEntry entry;
    auto it = m_offsetToFreeBlock.emplace(offset, entry);
    auto sizeIt = m_sizeToFreeBlocks.emplace(descriptorCount, it.first);
    it.first->second.m_sizeToFreeBlocksIt = sizeIt;
}

void DXDescriptorHeapPage::FreeSingleStale(UINT32 offset, UINT32 descriptorCount) {
    m_freeDescriptorCount += descriptorCount;

    auto previousIt = m_offsetToFreeBlock.lower_bound(offset);
    if (previousIt != m_offsetToFreeBlock.end()) {
        auto& entry = previousIt->second;
        auto size = entry.m_sizeToFreeBlocksIt->first;
        if (previousIt->first + size == offset) {
            offset = previousIt->first;
            descriptorCount += size;
            m_sizeToFreeBlocks.erase(entry.m_sizeToFreeBlocksIt);
            m_offsetToFreeBlock.erase(previousIt);
        }
    }

    auto nextIt = m_offsetToFreeBlock.upper_bound(offset);
    if (nextIt != m_offsetToFreeBlock.end()) {
        if (offset + descriptorCount == nextIt->first) {
            auto& entry = nextIt->second;
            auto size = entry.m_sizeToFreeBlocksIt->first;
            descriptorCount += size;
            m_sizeToFreeBlocks.erase(entry.m_sizeToFreeBlocksIt);
            m_offsetToFreeBlock.erase(nextIt->first);
        }
    }

    AddFreeBlock(offset, descriptorCount);
}

UINT32 DXDescriptorHeapPage::CalculateOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle) const {
    return (handle.ptr - m_rootDescriptorHandle.ptr) / m_descriptorIncreaseSize;
}

DXDescriptorHeapPage::StaleDescriptorsInfo::StaleDescriptorsInfo(UINT32 offset, UINT32 count, UINT64 frame)
    : m_offset(offset), m_count(count), m_frameNum(frame)
{}
