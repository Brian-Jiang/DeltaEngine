#include "Runtime/Graphics/DirectX/CommandList.h"

using namespace DeltaEngine;

void DeltaEngine::CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap)
{
    if (m_DescriptorHeaps[heapType] != heap) {
        m_DescriptorHeaps[heapType] = heap;

        UINT numDescriptorHeaps = 0;
        ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

        for (UINT32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
            ID3D12DescriptorHeap* descriptorHeap = m_DescriptorHeaps[i];
            if (descriptorHeap) {
                descriptorHeaps[numDescriptorHeaps++] = descriptorHeap;
            }
        }

        m_CommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
    }
}
