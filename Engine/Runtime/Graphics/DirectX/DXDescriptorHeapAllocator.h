#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <memory>
#include <vector>
#include <set>

#include "Graphics/DirectX/DXDescriptorHeapPage.h"
#include "Graphics/DirectX/DXDescriptorHeapAllocation.h"

DELTA_ENGINE_NS_BEGIN

class DXDescriptorHeapAllocator {
    DXDescriptorHeapAllocator(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT32 descriptorCountPerPage=128);

    DXDescriptorHeapAllocation Allocate(UINT32 descriptorCount);

private:
    D3D12_DESCRIPTOR_HEAP_TYPE m_heapType;
    std::vector<std::shared_ptr<DXDescriptorHeapPage>> m_pages;
    std::set<std::shared_ptr<DXDescriptorHeapPage>> m_availablePages;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    UINT32 m_descriptorCountPerPage;
};

DELTA_ENGINE_NS_END