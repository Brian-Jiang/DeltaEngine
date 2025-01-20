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
class UploadBuffer;
class Device;
class DynamicDescriptorHeap;
class RootSignature;
class ConstantBuffer;
class Buffer;
class Texture;
class ShaderResourceView;
class ConstantBufferView;
class UnorderedAccessView;

class CommandList : public std::enable_shared_from_this<CommandList>
{

public:
    CommandList(Device& device, D3D12_COMMAND_LIST_TYPE type);

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
     * Set a dynamic constant buffer data to an inline descriptor in the root
     * signature.
     */
    void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
    template<typename T>
    void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, const T& data) {
        SetGraphicsDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
    }

    /**
     * Set the current root signature on the command list.
     */
    void SetGraphicsRootSignature(const std::shared_ptr<RootSignature>& rootSignature);

    /**
     * Set an inline CBV.
     *
     * Note: Only ConstantBuffer's can be used with inline CBV's.
     */
    void SetConstantBufferView(uint32_t rootParameterIndex, const std::shared_ptr<ConstantBuffer>& buffer,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        size_t                bufferOffset = 0);

    /**
     * Set an inline SRV.
     *
     * Note: Only Buffer resources can be used with inline SRV's
     */
    void SetShaderResourceView(uint32_t rootParameterIndex, const std::shared_ptr<Buffer>& buffer,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        size_t bufferOffset = 0);
    /**
     * Set an inline UAV.
     *
     * Note: Only Buffer resoruces can be used with inline UAV's.
     */
    void SetUnorderedAccessView(uint32_t rootParameterIndex, const std::shared_ptr<Buffer>& buffer,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        size_t                bufferOffset = 0);

    /**
     * Set the CBV on the rendering pipeline.
     */
    void SetConstantBufferView(uint32_t rootParameterIndex, uint32_t descriptorOffset,
        const std::shared_ptr<ConstantBufferView>& cbv,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    /**
     * Set the SRV on the graphics pipeline.
     */
    void SetShaderResourceView(uint32_t rootParameterIndex, uint32_t descriptorOffset,
        const std::shared_ptr<ShaderResourceView>& srv,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        UINT firstSubresource = 0,
        UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    /**
     * Set an SRV on the graphics pipeline using the default SRV for the texture.
     */
    void SetShaderResourceView(int32_t rootParameterIndex, uint32_t descriptorOffset,
        const std::shared_ptr<Texture>& texture,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        UINT firstSubresource = 0,
        UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    /**
     * Set the UAV on the graphics pipeline.
     */
    void SetUnorderedAccessView(uint32_t rootParameterIndex, uint32_t descriptorOffset,
        const std::shared_ptr<UnorderedAccessView>& uav,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        UINT                  firstSubresource = 0,
        UINT                  numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    /**
     * Set the UAV on the graphics pipline using a specific mip of the texture.
     */
    void SetUnorderedAccessView(uint32_t rootParameterIndex, uint32_t descriptorOffset,
        const std::shared_ptr<Texture>& texture, UINT mip,
        D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        UINT                  firstSubresource = 0,
        UINT                  numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    /**
     * Draw geometry.
     */
    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t startVertex = 0, uint32_t startInstance = 0);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0,
        uint32_t startInstance = 0);

    /**
     * Resolve a multisampled resource into a non-multisampled resource.
     */
    void ResolveSubresource(const std::shared_ptr<Resource>&, const std::shared_ptr<Resource>&,
        uint32_t dstSubresource = 0, uint32_t srcSubresource = 0);

    /**
     * Reset the command list. This should only be called by the CommandQueue
     * before the command list is returned from CommandQueue::GetCommandList.
     */
    void Reset();

    /**
     * Close the command list.
     * Used by the command queue.
     *
     * @param pendingCommandList The command list that is used to execute pending
     * resource barriers (if any) for this command list.
     *
     * @return true if there are any pending resource barriers that need to be
     * processed.
     */
    bool Close(const std::shared_ptr<CommandList>& pendingCommandList);
    // Just close the command list. This is useful for pending command lists.
    void Close();

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

    // The device that is used to create this command list.
    Device& m_Device;
    D3D12_COMMAND_LIST_TYPE m_d3d12CommandListType;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_d3d12CommandAllocator;

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> m_d3d12CommandList;

    // Resource created in an upload heap. Useful for drawing of dynamic geometry
    // or for uploading constant buffer data that changes every draw call.
    std::unique_ptr<UploadBuffer> m_UploadBuffer;

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

    // Keep track of the currently bound root signatures to minimize root
    // signature changes.
    ID3D12RootSignature* m_RootSignature;

    // Keep track of the currently bond pipeline state object to minimize PSO changes.
    ID3D12PipelineState* m_PipelineState;

    // The dynamic descriptor heap allows for descriptors to be staged before
    // being committed to the command list. Dynamic descriptors need to be
    // committed before a Draw or Dispatch.
    std::unique_ptr<DynamicDescriptorHeap> m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
};

DELTA_ENGINE_NS_END