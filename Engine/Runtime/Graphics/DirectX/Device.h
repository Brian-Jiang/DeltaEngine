#pragma once

#include "EngineIncludes.h"

#include <wrl.h>
#include <memory>
#include <vector>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <xstring>

DELTA_ENGINE_NS_BEGIN

class DescriptorAllocator;
class DescriptorAllocation;
class CommandQueue;
class CommandList;
class DirectX12Texture;
class SwapChain;
class Adapter;
class ConstantBuffer;
class ByteAddressBuffer;
class StructuredBuffer;
class PipelineStateObject;
class RootSignature;
class ShaderResourceView;
class ConstantBufferView;
class UnorderedAccessView;
class Resource;
class IndexBuffer;
class VertexBuffer;

class Device : std::enable_shared_from_this<Device>
{

public:
    /**
     * Always enable the debug layer before doing anything DX12 related so all possible errors generated while creating
     * DX12 objects are caught by the debug layer.
     */
    static void EnableDebugLayer();

    static void ReportLiveObjects();

    /**
     * Create a new DX12 device using the provided adapter.
     * If no adapter is specified, then the highest performance adapter will be  chosen.
     */
    static std::shared_ptr<Device> Create(std::shared_ptr<Adapter> adapter = nullptr);

    /**
     * Get a description of the adapter that was used to create the device.
     */
    std::wstring GetDescription() const;

    /**
     * Allocate a number of CPU visible descriptors.
     */
    DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);

    /**
     * Gets the size of the handle increment for the given type of descriptor heap.
     */
    inline UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const
    {
        return m_d3d12Device->GetDescriptorHandleIncrementSize(type);
    }

    

    /**
     * Release upload resources that are no longer needed after GPU execution.
     * Should be called after command list execution completes.
     */
    //void ReleaseUploadResources();


    // ============================ Textures ============================
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


    // ============================ Swap Chain ============================

    /**
     * Create a swapchain using the provided OS window handle.
     */
    std::shared_ptr<SwapChain> CreateSwapChain(HWND hWnd, DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R10G10B10A2_UNORM);


    // ============================ Constant Buffer ============================

    /**
     * Create a ConstantBuffer from a given ID3D12Resoure.
     */
    std::shared_ptr<ConstantBuffer> CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> resource);


    // ============================ Byte Address Buffer ============================
    /**
     * Create a ByteAddressBuffer resource.
     *
     * @param resDesc A description of the resource.
     */
    std::shared_ptr<ByteAddressBuffer> CreateByteAddressBuffer(size_t bufferSize);
    std::shared_ptr<ByteAddressBuffer> CreateByteAddressBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> resource);


    // ============================ Structured Buffer ============================
    /**
     * Create a structured buffer resource.
     */
    std::shared_ptr<StructuredBuffer> CreateStructuredBuffer(size_t numElements, size_t elementSize);
    std::shared_ptr<StructuredBuffer> CreateStructuredBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> resource,
        size_t numElements, size_t elementSize);


    // ============================ Index Buffer ============================

    std::shared_ptr<IndexBuffer> CreateIndexBuffer(size_t numIndices, DXGI_FORMAT indexFormat);
    std::shared_ptr<IndexBuffer> CreateIndexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> resource, size_t numIndices,
        DXGI_FORMAT indexFormat);


    // ============================ Vertex Buffer ============================

    std::shared_ptr<VertexBuffer> CreateVertexBuffer(size_t numVertices, size_t vertexStride);
    std::shared_ptr<VertexBuffer> CreateVertexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> resource,
        size_t numVertices, size_t vertexStride);


    std::shared_ptr<RootSignature> CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc);

    template <class PipelineStateStream>
    std::shared_ptr<PipelineStateObject> CreatePipelineStateObject(PipelineStateStream& pipelineStateStream)
    {
        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream),
            &pipelineStateStream };

        return DoCreatePipelineStateObject(pipelineStateStreamDesc);
    }

    std::shared_ptr<ConstantBufferView> CreateConstantBufferView(const std::shared_ptr<ConstantBuffer>& constantBuffer,
        size_t offset = 0);

    std::shared_ptr<ShaderResourceView>
    CreateShaderResourceView(const std::shared_ptr<Resource>& resource,
        const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr);

    std::shared_ptr<UnorderedAccessView>
    CreateUnorderedAccessView(const std::shared_ptr<Resource>& resource,
        const std::shared_ptr<Resource>& counterResource = nullptr,
        const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav = nullptr);

    /**
     * Flush all command queues.
     */
    void Flush();

    /**
     * Release stale descriptors. This should only be called with a completed frame counter.
     */
    void ReleaseStaleDescriptors();

    /**
     * Get the adapter that was used to create this device.
     */
    inline std::shared_ptr<Adapter> GetAdapter() const
    {
        return m_Adapter;
    }

    /**
     * Get a command queue. Valid types are:
     * - D3D12_COMMAND_LIST_TYPE_DIRECT : Can be used for draw, dispatch, or copy commands.
     * - D3D12_COMMAND_LIST_TYPE_COMPUTE: Can be used for dispatch or copy commands.
     * - D3D12_COMMAND_LIST_TYPE_COPY   : Can be used for copy commands.
     * By default, a D3D12_COMMAND_LIST_TYPE_DIRECT queue is returned.
     */
    CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    inline Microsoft::WRL::ComPtr<ID3D12Device2> GetD3D12Device() const
    {
        return m_d3d12Device;
    }

    inline D3D_ROOT_SIGNATURE_VERSION GetHighestRootSignatureVersion() const
    {
        return m_HighestRootSignatureVersion;
    }

    /**
     * Check if the requested multisample quality is supported for the given format.
     */
    DXGI_SAMPLE_DESC GetMultisampleQualityLevels(
        DXGI_FORMAT format, UINT numSamples = D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT,
        D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE) const;


public:
    friend class DXRenderManager;

    explicit Device(std::shared_ptr<Adapter> adapter);
    virtual ~Device();

    std::shared_ptr<PipelineStateObject>
    DoCreatePipelineStateObject(const D3D12_PIPELINE_STATE_STREAM_DESC& pipelineStateStreamDesc);
    
private:
    Microsoft::WRL::ComPtr<ID3D12Device2> m_d3d12Device;

    // The adapter that was used to create the device:
    std::shared_ptr<Adapter> m_Adapter;

    // Default command queues.
    std::unique_ptr<CommandQueue> m_DirectCommandQueue;
    std::unique_ptr<CommandQueue> m_ComputeCommandQueue;
    std::unique_ptr<CommandQueue> m_CopyCommandQueue;

    // Descriptor allocators.
    std::unique_ptr<DescriptorAllocator> m_DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    D3D_ROOT_SIGNATURE_VERSION m_HighestRootSignatureVersion;
};

DELTA_ENGINE_NS_END
