#include "Graphics/DirectX/DXUploadBuffer.h"

#include "Math/Common.h"
#include "Graphics/DXUtils.h"

using namespace DeltaEngine;
using namespace std;



DXUploadBuffer::DXUploadBuffer(size_t pageSize)
    : m_pageSize(pageSize), m_currentPage(nullptr)
{

}

DXUploadBuffer::DXUploadBuffer(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t pageSize)
    : m_pageSize(pageSize), m_device(device), m_currentPage(nullptr)
{

}

void DXUploadBuffer::SetDevice(const Microsoft::WRL::ComPtr<ID3D12Device>& device) {
    m_device = device;
}

DXUploadBuffer::AddrPair DXUploadBuffer::Allocate(size_t sizeInBytes, size_t alignment) {
    if (!m_currentPage || !m_currentPage->HasSpace(sizeInBytes, alignment)) {
        if (!m_freePagePool.empty()) {
            m_currentPage = m_freePagePool.front();
            m_freePagePool.pop_front();
        }
        else {
            m_currentPage = make_shared<Page>(m_device, m_pageSize);
            m_pagePool.push_back(m_currentPage);
        }
    }

    return m_currentPage->Allocate(sizeInBytes, alignment);
}

void DXUploadBuffer::Reset() {
    m_freePagePool = m_pagePool;
    m_currentPage = nullptr;
    for (auto& page : m_pagePool) {
        page->Reset();
    }
}

DXUploadBuffer::Page::Page(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t size)
    : m_maxSize(size), m_offset(0), m_device(device)
{
    CD3DX12_HEAP_PROPERTIES heapProp(D3D12_HEAP_TYPE_UPLOAD);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_maxSize);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_dxResource)
    ));

    m_gpuAddr = m_dxResource->GetGPUVirtualAddress();
    ThrowIfFailed(m_dxResource->Map(0, nullptr, &m_cpuAddr));
}

DXUploadBuffer::Page::~Page()
{
    m_dxResource->Unmap(0, nullptr);
    m_offset = 0;
}

bool DXUploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
    auto alignedSize = AlignUp(sizeInBytes, alignment);
    auto alignedOffset = AlignUp(m_offset, alignment);
    return alignedOffset + alignedSize <= m_maxSize;
}

DXUploadBuffer::AddrPair DXUploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment) {
    auto alignedSize = AlignUp(sizeInBytes, alignment);
    auto alignedOffset = AlignUp(m_offset, alignment);
    
    AddrPair addr{ static_cast<UINT8*>(m_cpuAddr) + alignedOffset, m_gpuAddr + alignedOffset};
    
    m_offset = alignedOffset + alignedSize;

    return addr;
}

void DXUploadBuffer::Page::Reset() {
    m_offset = 0;
}
