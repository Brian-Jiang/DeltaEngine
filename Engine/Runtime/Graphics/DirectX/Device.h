#pragma once

#include "EngineIncludes.h"

#include <wrl.h>
#include <memory>
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>

DELTA_ENGINE_NS_BEGIN

class DescriptorAllocator;
class DescriptorAllocation;
class CommandQueue;
class CommandList;
class DirectX12Texture;

class Device : std::enable_shared_from_this<Device>
{

public:
    /**
     * Always enable the debug layer before doing anything DX12 related so all possible errors generated while creating
     * DX12 objects are caught by the debug layer.
     */
    static void EnableDebugLayer();

    static void ReportLiveObjects();

    //Device();
    //virtual ~Device() = default;


    /**
     * Flush all command queues.
     */
    void Flush();

    /**
     * Release stale descriptors. This should only be called with a completed frame counter.
     */
    void ReleaseStaleDescriptors();

    /**
     * Release upload resources that are no longer needed after GPU execution.
     * Should be called after command list execution completes.
     */
    void ReleaseUploadResources();

    /**
     * Create a Texture resource.
     *
     * @param resourceDesc A description of the texture to create.
     * @param [clearVlue] Optional optimized clear value for the texture.
     * @param [textureUsage] Optional texture usage flag provides a hint about how the texture will be used.
     *
     * @returns A pointer to the created texture.
     */
    std::shared_ptr<DirectX12Texture> CreateTexture(const D3D12_RESOURCE_DESC& resourceDesc,
        const D3D12_CLEAR_VALUE* clearValue = nullptr);
    std::shared_ptr<DirectX12Texture> CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource> resource,
        const D3D12_CLEAR_VALUE* clearValue = nullptr);

    /**
     * Upload a DTexture (loaded from file) to the GPU. Allocates descriptor and creates SRV internally.
     * Records copy and transition commands on the given command list.
     */
    void CreateTextureFromFile(class DTexture* dtex, CommandList& commandList);

    /**
     * Get the adapter that was used to create this device.
     */
    Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter() const {
        return m_Adapter;
    }

    /**
     * Allocate a number of CPU visible descriptors.
     */
    DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);

    /**
     * Get a command queue. Valid types are:
     * - D3D12_COMMAND_LIST_TYPE_DIRECT : Can be used for draw, dispatch, or copy commands.
     * - D3D12_COMMAND_LIST_TYPE_COMPUTE: Can be used for dispatch or copy commands.
     * - D3D12_COMMAND_LIST_TYPE_COPY   : Can be used for copy commands.
     * By default, a D3D12_COMMAND_LIST_TYPE_DIRECT queue is returned.
     */
    CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device() const {
        return m_d3d12Device;
    }

    D3D_ROOT_SIGNATURE_VERSION GetHighestRootSignatureVersion() const {
        return m_HighestRootSignatureVersion;
    }

    /**
     * Gets the size of the handle increment for the given type of descriptor heap.
     */
    UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const {
        return m_d3d12Device->GetDescriptorHandleIncrementSize(type);
    }


public:
    friend class DXRenderManager;

    explicit Device(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);
    virtual ~Device();
    
private:
    Microsoft::WRL::ComPtr<ID3D12Device2> m_d3d12Device;

    // The adapter that was used to create the device:
    Microsoft::WRL::ComPtr<IDXGIAdapter4> m_Adapter;

    // Default command queues.
    std::unique_ptr<CommandQueue> m_DirectCommandQueue;
    std::unique_ptr<CommandQueue> m_ComputeCommandQueue;
    std::unique_ptr<CommandQueue> m_CopyCommandQueue;

    // Descriptor allocators.
    std::unique_ptr<DescriptorAllocator> m_DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    D3D_ROOT_SIGNATURE_VERSION m_HighestRootSignatureVersion;

    // Track in-flight upload resources that must stay alive until GPU execution completes.
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_InFlightUploadResources;
};

DELTA_ENGINE_NS_END