#include "Graphics/DirectX/DXDescriptorHeapAllocator.h"

using namespace std;
using namespace DeltaEngine;

DXDescriptorHeapAllocation DXDescriptorHeapAllocator::Allocate(UINT32 descriptorCount) {
	if (m_availablePages.empty()) {
		auto page = make_shared<DXDescriptorHeapPage>(m_device, m_descriptorCountPerPage, m_heapType);
	}
    return DXDescriptorHeapAllocation();
}
