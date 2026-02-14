#include "Runtime/Graphics/DirectX/Device.h"

#include <assert.h>
#include <d3dx12.h>
#include <dxgidebug.h>

#include "Runtime/Core/DTexture.h"
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/DescriptorAllocation.h"
#include "Runtime/Graphics/DirectX/DescriptorAllocator.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/SwapChain.h"
#include "Runtime/Graphics/DirectX/Adapter.h"
#include "Runtime/Graphics/DirectX/ConstantBuffer.h"
#include "Runtime/Graphics/DirectX/UnorderedAccessView.h"
#include "Runtime/Graphics/DirectX/ShaderResourceView.h"
#include "Runtime/Graphics/DirectX/ConstantBufferView.h"
#include "Runtime/Graphics/DirectX/PipelineStateObject.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/DirectX/StructuredBuffer.h"
#include "Runtime/Graphics/DirectX/VertexBuffer.h"
#include "Runtime/Graphics/DirectX/IndexBuffer.h"
#include "Runtime/Graphics/DXUtils.h"
#include "Runtime/Math/Common.h"

using namespace DeltaEngine;
using namespace Microsoft::WRL;

class MakeUnorderedAccessView : public UnorderedAccessView
{
public:
    MakeUnorderedAccessView(Device& device, const std::shared_ptr<Resource>& resource,
        const std::shared_ptr<Resource>& counterResource,
        const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav)
        : UnorderedAccessView(device, resource, counterResource, uav)
    {
    }

    virtual ~MakeUnorderedAccessView() { }
};

class MakeShaderResourceView : public ShaderResourceView
{
public:
    MakeShaderResourceView(Device& device, const std::shared_ptr<Resource>& resource,
        const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
        : ShaderResourceView(device, resource, srv)
    {
    }

    virtual ~MakeShaderResourceView() { }
};

class MakeConstantBufferView : public ConstantBufferView
{
public:
    MakeConstantBufferView(Device& device, const std::shared_ptr<ConstantBuffer>& constantBuffer, size_t offset)
        : ConstantBufferView(device, constantBuffer, offset)
    {
    }

    virtual ~MakeConstantBufferView() { }
};

class MakePipelineStateObject : public PipelineStateObject
{
public:
    MakePipelineStateObject(Device& device, const D3D12_PIPELINE_STATE_STREAM_DESC& desc)
        : PipelineStateObject(device, desc)
    {
    }

    virtual ~MakePipelineStateObject() { }
};
class MakeRootSignature : public RootSignature
{
public:
    MakeRootSignature(Device& device, const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc)
        : RootSignature(device, rootSignatureDesc)
    {
    }

    virtual ~MakeRootSignature() { }
};

class MakeDirectX12Texture : public DirectX12Texture
{
public:
    MakeDirectX12Texture(Device& device, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue)
        : DirectX12Texture(device, resourceDesc, clearValue)
    {
    }

    MakeDirectX12Texture(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue)
        : DirectX12Texture(device, resource, clearValue)
    {
    }

    virtual ~MakeDirectX12Texture() { }
};

class MakeStructuredBuffer : public StructuredBuffer {
public:
    MakeStructuredBuffer(Device& device, size_t numElements, size_t elementSize)
        : StructuredBuffer(device, numElements, elementSize)
    {
    }

    MakeStructuredBuffer(Device& device, ComPtr<ID3D12Resource> resource, size_t numElements, size_t elementSize)
        : StructuredBuffer(device, resource, numElements, elementSize)
    {
    }

    virtual ~MakeStructuredBuffer() { }
};

class MakeVertexBuffer : public VertexBuffer {
public:
    MakeVertexBuffer(Device& device, size_t numVertices, size_t vertexStride)
        : VertexBuffer(device, numVertices, vertexStride)
    {
    }

    MakeVertexBuffer(Device& device, ComPtr<ID3D12Resource> resource, size_t numVertices, size_t vertexStride)
        : VertexBuffer(device, resource, numVertices, vertexStride)
    {
    }

    virtual ~MakeVertexBuffer() { }
};

class MakeIndexBuffer : public IndexBuffer {
public:
    MakeIndexBuffer(Device& device, size_t numIndices, DXGI_FORMAT indexFormat)
        : IndexBuffer(device, numIndices, indexFormat)
    {
    }

    MakeIndexBuffer(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, size_t numIndices,
        DXGI_FORMAT indexFormat)
        : IndexBuffer(device, resource, numIndices, indexFormat)
    {
    }

    virtual ~MakeIndexBuffer() = default;
};

class MakeConstantBuffer : public ConstantBuffer {
public:
    MakeConstantBuffer(Device& device, ComPtr<ID3D12Resource> resource)
        : ConstantBuffer(device, resource)
    {
    }

    virtual ~MakeConstantBuffer() = default;
};

class MakeByteAddressBuffer : public ByteAddressBuffer {
public:
    MakeByteAddressBuffer(Device& device, const D3D12_RESOURCE_DESC& desc)
        : ByteAddressBuffer(device, desc)
    {
    }

    MakeByteAddressBuffer(Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource)
        : ByteAddressBuffer(device, resource)
    {
    }

    virtual ~MakeByteAddressBuffer() = default;
};

class MakeDescriptorAllocator : public DescriptorAllocator {
public:
    MakeDescriptorAllocator(Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256)
        : DescriptorAllocator(device, type, numDescriptorsPerHeap)
    {
    }

    virtual ~MakeDescriptorAllocator() { }
};

class MakeSwapChain : public SwapChain
{
public:
    MakeSwapChain(Device& device, HWND hWnd, DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R10G10B10A2_UNORM)
        : SwapChain(device, hWnd, backBufferFormat)
    {
    }

    virtual ~MakeSwapChain() { }
};

class MakeCommandQueue : public CommandQueue
{
public:
    MakeCommandQueue(Device& device, D3D12_COMMAND_LIST_TYPE type)
        : CommandQueue(device, type)
    {
    }

    virtual ~MakeCommandQueue() { }
};

class MakeDevice : public Device
{
public:
    MakeDevice(std::shared_ptr<Adapter> adapter)
        : Device(adapter)
    {
    }

    virtual ~MakeDevice() { }
};

void Device::EnableDebugLayer()
{
    ComPtr<ID3D12Debug> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
}

void Device::ReportLiveObjects()
{
    IDXGIDebug1* dxgiDebug;
    ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)));

    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    dxgiDebug->Release();
}

std::shared_ptr<Device> Device::Create(std::shared_ptr<Adapter> adapter)
{
    return std::make_shared<MakeDevice>(adapter);
}

std::wstring Device::GetDescription() const
{
    return m_Adapter->GetDescription();
}

Device::Device(std::shared_ptr<Adapter> adapter)
    : m_Adapter(adapter)
{
    if (!m_Adapter)
    {
        m_Adapter = Adapter::Create();
        assert(m_Adapter);
    }

    auto dxgiAdapter = m_Adapter->GetDXGIAdapter();

    ThrowIfFailed(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3d12Device)));

    // Enable debug messages (only works if the debug layer has already been enabled).
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(m_d3d12Device.As(&pInfoQueue))) {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Suppress whole categories of messages
        // D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] = { D3D12_MESSAGE_SEVERITY_INFO };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE, // I'm really not sure how to avoid this
                                                                          // message.

            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE, // This warning occurs when using capture frame while graphics
                                                    // debugging.

            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE, // This warning occurs when using capture frame while graphics
                                                      // debugging.
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        // NewFilter.DenyList.NumCategories = _countof(Categories);
        // NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
    }

    m_DirectCommandQueue = std::make_unique<CommandQueue>(*this, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_ComputeCommandQueue = std::make_unique<CommandQueue>(*this, D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_CopyCommandQueue = std::make_unique<CommandQueue>(*this, D3D12_COMMAND_LIST_TYPE_COPY);

    // Create descriptor allocators
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        m_DescriptorAllocators[i] =
            std::make_unique<DescriptorAllocator>(*this, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
    }

    // Check features.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData;
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData,
            sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        m_HighestRootSignatureVersion = featureData.HighestVersion;
    }
}

DeltaEngine::Device::~Device() {}

CommandQueue& Device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
    CommandQueue* commandQueue;
    switch (type)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        commandQueue = m_DirectCommandQueue.get();
        break;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        commandQueue = m_ComputeCommandQueue.get();
        break;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        commandQueue = m_CopyCommandQueue.get();
        break;
    default:
        assert(false && "Invalid command queue type.");
    }

    return *commandQueue;
}

void Device::Flush() {
    m_DirectCommandQueue->Flush();
    m_ComputeCommandQueue->Flush();
    m_CopyCommandQueue->Flush();
}

DescriptorAllocation Device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    return m_DescriptorAllocators[type]->Allocate(numDescriptors);
}

void Device::ReleaseStaleDescriptors()
{
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        m_DescriptorAllocators[i]->ReleaseStaleDescriptors();
    }
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Device::CreateShaderVisibleSrvHeap(uint32_t numDescriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = numDescriptors;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> heap;
    ThrowIfFailed(m_d3d12Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
    return heap;
}

std::shared_ptr<SwapChain> Device::CreateSwapChain(HWND hWnd, DXGI_FORMAT backBufferFormat)
{
    auto swapChain = std::make_shared<MakeSwapChain>(*this, hWnd, backBufferFormat);
    return swapChain;
}

std::shared_ptr<ConstantBuffer> Device::CreateConstantBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> resource)
{
    std::shared_ptr<ConstantBuffer> constantBuffer = std::make_shared<MakeConstantBuffer>(*this, resource);
    return constantBuffer;
}

std::shared_ptr<ByteAddressBuffer> Device::CreateByteAddressBuffer(size_t bufferSize)
{
    // Align-up to 4-bytes
    bufferSize = AlignUp(bufferSize, 4);

    std::shared_ptr<ByteAddressBuffer> buffer = std::make_shared<MakeByteAddressBuffer>(
        *this, CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS));

    return buffer;
}

std::shared_ptr<ByteAddressBuffer> Device::CreateByteAddressBuffer(ComPtr<ID3D12Resource> resource)
{
    std::shared_ptr<ByteAddressBuffer> buffer = std::make_shared<MakeByteAddressBuffer>(*this, resource);

    return buffer;
}

std::shared_ptr<StructuredBuffer> Device::CreateStructuredBuffer(size_t numElements, size_t elementSize)
{
    std::shared_ptr<StructuredBuffer> structuredBuffer = std::make_shared<MakeStructuredBuffer>(*this, numElements, elementSize);

    return structuredBuffer;
}

std::shared_ptr<StructuredBuffer> Device::CreateStructuredBuffer(ComPtr<ID3D12Resource> resource, size_t numElements,
    size_t elementSize)
{
    std::shared_ptr<StructuredBuffer> structuredBuffer = std::make_shared<MakeStructuredBuffer>(*this, resource, numElements, elementSize);

    return structuredBuffer;
}

std::shared_ptr<IndexBuffer> Device::CreateIndexBuffer(size_t numIndices, DXGI_FORMAT indexFormat)
{
    std::shared_ptr<IndexBuffer> indexBuffer = std::make_shared<MakeIndexBuffer>(*this, numIndices, indexFormat);

    return indexBuffer;
}

std::shared_ptr<IndexBuffer> Device::CreateIndexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> resource, size_t numIndices,
    DXGI_FORMAT indexFormat)
{
    std::shared_ptr<IndexBuffer> indexBuffer = std::make_shared<MakeIndexBuffer>(*this, resource, numIndices, indexFormat);

    return indexBuffer;
}

std::shared_ptr<VertexBuffer> Device::CreateVertexBuffer(size_t numVertices, size_t vertexStride)
{
    std::shared_ptr<VertexBuffer> vertexBuffer = std::make_shared<MakeVertexBuffer>(*this, numVertices, vertexStride);

    return vertexBuffer;
}

std::shared_ptr<VertexBuffer> Device::CreateVertexBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> resource, size_t numVertices,
    size_t vertexStride)
{
    std::shared_ptr<VertexBuffer> vertexBuffer = std::make_shared<MakeVertexBuffer>(*this, resource, numVertices, vertexStride);

    return vertexBuffer;
}

std::shared_ptr<DirectX12Texture> Device::CreateTexture(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue)
{
    return std::make_shared<MakeDirectX12Texture>(*this, resourceDesc, clearValue);
}

std::shared_ptr<DirectX12Texture> Device::CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue)
{
    return std::make_shared<MakeDirectX12Texture>(*this, resource, clearValue);
}

std::shared_ptr<RootSignature> Device::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc)
{
    std::shared_ptr<RootSignature> rootSignature = std::make_shared<MakeRootSignature>(*this, rootSignatureDesc);

    return rootSignature;
}

std::shared_ptr<PipelineStateObject> Device::DoCreatePipelineStateObject(
    const D3D12_PIPELINE_STATE_STREAM_DESC& pipelineStateStreamDesc)
{
    std::shared_ptr<PipelineStateObject> pipelineStateObject = std::make_shared<MakePipelineStateObject>(*this, pipelineStateStreamDesc);

    return pipelineStateObject;
}

std::shared_ptr<ConstantBufferView> Device::CreateConstantBufferView(const std::shared_ptr<ConstantBuffer>& constantBuffer, size_t offset)
{
    std::shared_ptr<ConstantBufferView> constantBufferView = std::make_shared<MakeConstantBufferView>(*this, constantBuffer, offset);

    return constantBufferView;
}

std::shared_ptr<ShaderResourceView> Device::CreateShaderResourceView(const std::shared_ptr<Resource>& resource,
    const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
{
    std::shared_ptr<ShaderResourceView> shaderResourceView = std::make_shared<MakeShaderResourceView>(*this, resource, srv);

    return shaderResourceView;
}

std::shared_ptr<UnorderedAccessView> Device::CreateUnorderedAccessView(const std::shared_ptr<Resource>& resource,
    const std::shared_ptr<Resource>& counterResource,
    const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav)
{
    std::shared_ptr<UnorderedAccessView> unorderedAccessView = std::make_shared<MakeUnorderedAccessView>(*this, resource, counterResource, uav);

    return unorderedAccessView;
}

//void Device::ReleaseUploadResources()
//{
//    m_InFlightUploadResources.clear();
//}
//
//void Device::CreateTextureFromFile(DTexture* dtex, CommandList& commandList)
//{
//    if (!dtex || dtex->GetData().empty())
//        return;
//
//    auto d3d12Device = GetD3D12Device();
//    UINT textureWidth = dtex->GetWidth();
//    UINT textureHeight = dtex->GetHeight();
//    const UINT pixelSize = 4;
//    UINT64 rowPitch = textureWidth * pixelSize;
//    UINT64 alignedRowPitch = (rowPitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
//    UINT64 textureSize = alignedRowPitch * textureHeight;
//
//    D3D12_RESOURCE_DESC textureDesc = {};
//    textureDesc.MipLevels = 1;
//    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//    textureDesc.Width = textureWidth;
//    textureDesc.Height = textureHeight;
//    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
//    textureDesc.DepthOrArraySize = 1;
//    textureDesc.SampleDesc.Count = 1;
//    textureDesc.SampleDesc.Quality = 0;
//    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//
//    ComPtr<ID3D12Resource> textureResource;
//    CD3DX12_HEAP_PROPERTIES heapDefault(D3D12_HEAP_TYPE_DEFAULT);
//    ThrowIfFailed(d3d12Device->CreateCommittedResource(
//        &heapDefault,
//        D3D12_HEAP_FLAG_NONE,
//        &textureDesc,
//        D3D12_RESOURCE_STATE_COPY_DEST,
//        nullptr,
//        IID_PPV_ARGS(&textureResource)));
//
//    ComPtr<ID3D12Resource> uploadHeap;
//    CD3DX12_HEAP_PROPERTIES heapUpload(D3D12_HEAP_TYPE_UPLOAD);
//    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(textureSize);
//    ThrowIfFailed(d3d12Device->CreateCommittedResource(
//        &heapUpload,
//        D3D12_HEAP_FLAG_NONE,
//        &bufferDesc,
//        D3D12_RESOURCE_STATE_GENERIC_READ,
//        nullptr,
//        IID_PPV_ARGS(&uploadHeap)));
//
//    const unsigned char* rawData = dtex->GetData().data();
//    UINT8* pData = nullptr;
//    uploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pData));
//    for (UINT y = 0; y < textureHeight; ++y)
//        memcpy(pData + y * alignedRowPitch, rawData + y * rowPitch, rowPitch);
//    uploadHeap->Unmap(0, nullptr);
//
//    D3D12_SUBRESOURCE_FOOTPRINT pitchedDesc = {};
//    pitchedDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//    pitchedDesc.Width = textureWidth;
//    pitchedDesc.Height = textureHeight;
//    pitchedDesc.Depth = 1;
//    pitchedDesc.RowPitch = static_cast<UINT>(alignedRowPitch);
//
//    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed = { 0 };
//    placed.Offset = 0;
//    placed.Footprint = pitchedDesc;
//
//    D3D12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(textureResource.Get(), 0);
//    D3D12_TEXTURE_COPY_LOCATION src = CD3DX12_TEXTURE_COPY_LOCATION(uploadHeap.Get(), placed);
//    commandList.CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
//
//    // Keep the upload heap alive until the command list is executed.
//    m_InFlightUploadResources.push_back(uploadHeap);
//
//    D3D12_RESOURCE_BARRIER rb = CD3DX12_RESOURCE_BARRIER::Transition(
//        textureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
//    commandList.ResourceBarrier(1, &rb);
//
//    std::shared_ptr<DirectX12Texture> dx12Texture = std::make_shared<DirectX12Texture>(*this, textureResource, nullptr);
//    dtex->SetGPUTexture(dx12Texture);
//}



DXGI_SAMPLE_DESC Device::GetMultisampleQualityLevels(DXGI_FORMAT format, UINT numSamples,
    D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags) const
{
    DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS qualityLevels;
    qualityLevels.Format = format;
    qualityLevels.SampleCount = 1;
    qualityLevels.Flags = flags;
    qualityLevels.NumQualityLevels = 0;

    while (
        qualityLevels.SampleCount <= numSamples && SUCCEEDED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &qualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS))) && qualityLevels.NumQualityLevels > 0)
    {
        // That works...
        sampleDesc.Count = qualityLevels.SampleCount;
        sampleDesc.Quality = qualityLevels.NumQualityLevels - 1;

        // But can we do better?
        qualityLevels.SampleCount *= 2;
    }

    return sampleDesc;
}