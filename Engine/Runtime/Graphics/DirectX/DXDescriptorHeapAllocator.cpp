#include "Graphics/DirectX/DXDescriptorHeapAllocator.h"

using namespace std;
using namespace DeltaEngine;

DXDescriptorHeapAllocator::DXDescriptorHeapAllocator(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT32 descriptorCountPerPage)
	: m_heapType(heapType), m_descriptorCountPerPage(descriptorCountPerPage)
{

}

void DXDescriptorHeapAllocator::SetDevice(Microsoft::WRL::ComPtr<ID3D12Device> device) {
	m_device = device;
}

DXDescriptorHeapAllocation DXDescriptorHeapAllocator::Allocate(UINT32 descriptorCount) {
	if (descriptorCount > m_descriptorCountPerPage) {
		return DXDescriptorHeapAllocation();
	}

	shared_ptr<DXDescriptorHeapPage> page;
	DXDescriptorHeapAllocation allocation;
	if (m_availablePages.empty()) {
		page = make_shared<DXDescriptorHeapPage>(m_device, m_descriptorCountPerPage, m_heapType);
		m_pages.emplace_back(page);
		m_availablePages.emplace(page);
		allocation = page->Allocate(descriptorCount);
	}
	else {
		for (auto& pageCandidate : m_availablePages) {
			allocation = pageCandidate->Allocate(descriptorCount);
			if (!allocation.IsNull()) {
				page = pageCandidate;
				break;
			}
		}
	}

	if (page->GetFreeDescriptorCount() <= 0) {
		m_availablePages.erase(page);
	}

	return allocation;
}

void DXDescriptorHeapAllocator::ReleaseAllStale(UINT64 frame) {
	for (auto& page : m_pages) {
		page->ReleaseAllStale(frame);
	}
}
