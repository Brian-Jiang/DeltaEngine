#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <vector>

//#include "Runtime/Core/SceneComponent.h"
//#include "Runtime/Graphics/DXGraphicsContext.h"

DELTA_ENGINE_NS_BEGIN

class Resource;
class ResourceStateTracker;
class DXUploadBuffer;

class CommandList : public std::enable_shared_from_this<CommandList>
{

public:
    void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

    /**
     * Transition a resource to a particular state.
     *
     * @param resource The resource to transition.
     * @param stateAfter The state to transition the resource to. The before state is resolved by the resource state
     * tracker.
     * @param subresource The subresource to transition. By default, this is D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES
     * which indicates that all subresources are transitioned to the same state.
     * @param flushBarriers Force flush any barriers. Resource barriers need to be flushed before a command (draw,
     * dispatch, or copy) that expects the resource to be in a particular state can run.
     */
    void TransitionBarrier(const std::shared_ptr<Resource>& resource, D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
    void TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);

    /**
     * Copy resources.
     */
    void CopyResource(const std::shared_ptr<Resource>& dstRes, const std::shared_ptr<Resource>& srcRes);
    void CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> srcRes);

    /**
     * Flush any barriers that have been pushed to the command list.
     */
    void FlushResourceBarriers();

    /**
     * Get direct access to the ID3D12GraphicsCommandList2 interface.
     */
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> GetD3D12CommandList() const {
        return m_d3d12CommandList;
    }
    
    
private:
    // Add a resource to a list of tracked resources (ensures lifetime while command list is in-flight on a command
    // queue.
    void TrackResource(Microsoft::WRL::ComPtr<ID3D12Object> object);
    void TrackResource(const std::shared_ptr<Resource>& res);

    /**
     * Release tracked objects. Useful if the swap chain needs to be resized.
     */
    void ReleaseTrackedObjects();

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> m_d3d12CommandList;

    // Resource created in an upload heap. Useful for drawing of dynamic geometry
    // or for uploading constant buffer data that changes every draw call.
    std::unique_ptr<DXUploadBuffer> m_UploadBuffer;

    // Resource state tracker is used by the command list to track (per command list)
    // the current state of a resource. The resource state tracker also tracks the
    // global state of a resource in order to minimize resource state transitions.
    std::unique_ptr<ResourceStateTracker> m_ResourceStateTracker;

    // Keep track of the currently bound descriptor heaps. Only change descriptor 
    // heaps if they are different than the currently bound descriptor heaps.
    ID3D12DescriptorHeap* m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    using TrackedObjects = std::vector<Microsoft::WRL::ComPtr<ID3D12Object>>;

    // Objects that are being tracked by a command list that is "in-flight" on
    // the command-queue and cannot be deleted. To ensure objects are not deleted
    // until the command list is finished executing, a reference to the object
    // is stored. The referenced objects are released when the command list is
    // reset.
    TrackedObjects m_TrackedObjects;
};

DELTA_ENGINE_NS_END