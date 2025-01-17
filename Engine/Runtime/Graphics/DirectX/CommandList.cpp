#include "Runtime/Graphics/DirectX/CommandList.h"

#include <d3dx12.h>
#include <assert.h>
#include "Runtime/Graphics/DirectX/Resource.h"
#include "Runtime/Graphics/DirectX/ResourceStateTracker.h"

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

        m_d3d12CommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
    }
}

void CommandList::TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter,
    UINT subresource, bool flushBarriers) {
    if (resource) {
        // The "before" state is not important. It will be resolved by the resource state tracker.
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter,
            subresource);
        m_ResourceStateTracker->ResourceBarrier(barrier);
    }

    if (flushBarriers) {
        FlushResourceBarriers();
    }
}

void CommandList::TransitionBarrier(const std::shared_ptr<Resource>& resource, D3D12_RESOURCE_STATES stateAfter,
    UINT subresource, bool flushBarriers) {
    if (resource) {
        TransitionBarrier(resource->GetD3D12Resource(), stateAfter, subresource, flushBarriers);
    }
}

void CommandList::CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstRes,
    Microsoft::WRL::ComPtr<ID3D12Resource> srcRes) {
    assert(dstRes);
    assert(srcRes);

    TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

    FlushResourceBarriers();

    m_d3d12CommandList->CopyResource(dstRes.Get(), srcRes.Get());

    TrackResource(dstRes);
    TrackResource(srcRes);
}

void CommandList::CopyResource(const std::shared_ptr<Resource>& dstRes, const std::shared_ptr<Resource>& srcRes) {
    assert(dstRes && srcRes);

    CopyResource(dstRes->GetD3D12Resource(), srcRes->GetD3D12Resource());
}

void CommandList::TrackResource(Microsoft::WRL::ComPtr<ID3D12Object> object) {
    m_TrackedObjects.push_back(object);
}

void CommandList::TrackResource(const std::shared_ptr<Resource>& res) {
    assert(res);

    TrackResource(res->GetD3D12Resource());
}

void CommandList::ReleaseTrackedObjects() {
    m_TrackedObjects.clear();
}


void CommandList::FlushResourceBarriers() {
    m_ResourceStateTracker->FlushResourceBarriers(shared_from_this());
}