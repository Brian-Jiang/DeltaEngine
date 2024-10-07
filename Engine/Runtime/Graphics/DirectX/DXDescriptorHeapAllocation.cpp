#include "Graphics/DirectX/DXDescriptorHeapAllocation.h"

#include "Graphics/DirectX/DXDescriptorHeapPage.h"

using namespace DeltaEngine;

DXDescriptorHeapAllocation::DXDescriptorHeapAllocation() {}

DXDescriptorHeapAllocation::DXDescriptorHeapAllocation(DXDescriptorHeapAllocation&& other)
    : m_rootDescriptorHandle(other.m_rootDescriptorHandle), m_descriptorIncreaseSize(other.m_descriptorIncreaseSize),
    m_descriptorCount(other.m_descriptorCount), m_descriptorHeapPage(other.m_descriptorHeapPage)
{
}

DXDescriptorHeapAllocation& DXDescriptorHeapAllocation::operator=(DXDescriptorHeapAllocation&& other) {
    Free();

    m_rootDescriptorHandle = other.m_rootDescriptorHandle;
    m_descriptorIncreaseSize = other.m_descriptorIncreaseSize;
    m_descriptorCount = other.m_descriptorCount;
    m_descriptorHeapPage = other.m_descriptorHeapPage;

    return *this;
}

D3D12_CPU_DESCRIPTOR_HANDLE DXDescriptorHeapAllocation::GetDescriptorHandle(UINT32 offset) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle{ m_rootDescriptorHandle.ptr + offset * m_descriptorIncreaseSize };
    return handle;
}

void DXDescriptorHeapAllocation::Free() {

}
