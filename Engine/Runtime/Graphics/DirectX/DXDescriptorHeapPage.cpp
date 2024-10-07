#include "DXDescriptorHeapPage.h"

#include "Graphics/DXUtils.h"
#include "Graphics/DirectX/DXDescriptorHeapAllocation.h"

using namespace DeltaEngine;

DXDescriptorHeapPage::DXDescriptorHeapPage(Microsoft::WRL::ComPtr<ID3D12Device> device, UINT32 totalDescriptorCount, D3D12_DESCRIPTOR_HEAP_TYPE heapType) 
    : m_heapType(heapType)
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
    return DXDescriptorHeapAllocation();
}

void DXDescriptorHeapPage::AddFreeBlock(UINT32 offset, UINT32 descriptorCount) {

}
