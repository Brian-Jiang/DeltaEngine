#include "Graphics/DirectX/ImGuiSrvDescriptorAllocator.h"
#include "Graphics/DirectX/Device.h"

using namespace DeltaEngine;

void ImGuiSrvDescriptorAllocator::Create(Device& device, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap)
{
    m_heap = std::move(heap);
    D3D12_DESCRIPTOR_HEAP_DESC desc = m_heap->GetDesc();
    m_heapStartCpu = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_heapStartGpu = m_heap->GetGPUDescriptorHandleForHeapStart();
    m_heapHandleIncrement = device.GetDescriptorHandleIncrementSize(desc.Type);
    m_freeIndices.clear();
    m_freeIndices.reserve(static_cast<size_t>(desc.NumDescriptors));
    for (int n = static_cast<int>(desc.NumDescriptors); n > 0; --n)
        m_freeIndices.push_back(n - 1);
}

void ImGuiSrvDescriptorAllocator::Destroy()
{
    m_heap.Reset();
    m_freeIndices.clear();
}

void ImGuiSrvDescriptorAllocator::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
{
    int idx = m_freeIndices.back();
    m_freeIndices.pop_back();
    outCpuHandle->ptr = m_heapStartCpu.ptr + (idx * m_heapHandleIncrement);
    outGpuHandle->ptr = m_heapStartGpu.ptr + (idx * m_heapHandleIncrement);
}

void ImGuiSrvDescriptorAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
    int cpuIdx = static_cast<int>((cpuHandle.ptr - m_heapStartCpu.ptr) / m_heapHandleIncrement);
    m_freeIndices.push_back(cpuIdx);
}
