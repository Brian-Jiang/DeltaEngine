#include "Graphics/DirectX/DXDescriptorHeapAllocation.h"

#include "Graphics/DirectX/DXDescriptorHeapPage.h"
#include "Core/Time.h"

using namespace std;
using namespace DeltaEngine;

DXDescriptorHeapAllocation::DXDescriptorHeapAllocation()
    : m_rootDescriptorHandle(), m_descriptorIncreaseSize(0), m_descriptorCount(0), m_descriptorHeapPage(nullptr)
{
}

DXDescriptorHeapAllocation::DXDescriptorHeapAllocation(D3D12_CPU_DESCRIPTOR_HANDLE handle, UINT32 size, UINT increaseSize, shared_ptr<DXDescriptorHeapPage> page)
    : m_rootDescriptorHandle(handle), m_descriptorIncreaseSize(increaseSize), m_descriptorCount(size), m_descriptorHeapPage(page)
{
}

DXDescriptorHeapAllocation::DXDescriptorHeapAllocation(DXDescriptorHeapAllocation&& other)
    : m_rootDescriptorHandle(other.m_rootDescriptorHandle), m_descriptorIncreaseSize(other.m_descriptorIncreaseSize),
    m_descriptorCount(other.m_descriptorCount), m_descriptorHeapPage(move(other.m_descriptorHeapPage))
{
    other.m_rootDescriptorHandle = D3D12_CPU_DESCRIPTOR_HANDLE();
    other.m_descriptorIncreaseSize = 0;
    other.m_descriptorCount = 0;
}

DXDescriptorHeapAllocation::~DXDescriptorHeapAllocation() {
    Free();
}

DXDescriptorHeapAllocation& DXDescriptorHeapAllocation::operator=(DXDescriptorHeapAllocation&& other) {
    Free();

    m_rootDescriptorHandle = other.m_rootDescriptorHandle;
    m_descriptorIncreaseSize = other.m_descriptorIncreaseSize;
    m_descriptorCount = other.m_descriptorCount;
    m_descriptorHeapPage = move(other.m_descriptorHeapPage);

    other.m_rootDescriptorHandle = D3D12_CPU_DESCRIPTOR_HANDLE();
    other.m_descriptorIncreaseSize = 0;
    other.m_descriptorCount = 0;

    return *this;
}

D3D12_CPU_DESCRIPTOR_HANDLE DXDescriptorHeapAllocation::GetDescriptorHandle(UINT32 offset) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle{ m_rootDescriptorHandle.ptr + offset * m_descriptorIncreaseSize };
    return handle;
}

UINT32 DXDescriptorHeapAllocation::GetDescriptorCount() {
    return m_descriptorCount;
}

bool DXDescriptorHeapAllocation::IsNull() const {
    return m_rootDescriptorHandle.ptr == 0;
}

void DXDescriptorHeapAllocation::Free() {
    if (IsNull() || !m_descriptorHeapPage) {
        return;
    }

    auto frame = Time::frameSinceStart;
    m_descriptorHeapPage->Release(move(*this), frame);

    m_rootDescriptorHandle = D3D12_CPU_DESCRIPTOR_HANDLE();
    m_descriptorIncreaseSize = 0;
    m_descriptorCount = 0;
    m_descriptorHeapPage.reset();
}
