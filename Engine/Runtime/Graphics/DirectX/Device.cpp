#include "Runtime/Graphics/DirectX/Device.h"

#include <assert.h>
#include <d3dx12.h>
#include <dxgidebug.h>

#include "Runtime/Graphics/DTexture.h"
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/DescriptorAllocation.h"
#include "Runtime/Graphics/DirectX/DescriptorAllocator.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/Texture.h"
#include "Runtime/Graphics/DXUtils.h"

using namespace DeltaEngine;
using namespace Microsoft::WRL;

void Device::EnableDebugLayer() {
    ComPtr<ID3D12Debug> debugInterface;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
}

void Device::ReportLiveObjects() {

    IDXGIDebug1* dxgiDebug;
    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

    dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
    dxgiDebug->Release();
}

Device::Device(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
    : m_Adapter(adapter) {
    auto& dxgiAdapter = m_Adapter;

    ThrowIfFailed(D3D12CreateDevice(dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3d12Device)));

    // Enable debug messages (only works if the debug layer has already been enabled).
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(m_d3d12Device.As(&pInfoQueue))) {
        //pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE );
        //pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE );
        //pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE );

        // Suppress whole categories of messages
        // D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] = { D3D12_MESSAGE_SEVERITY_INFO };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,  // I'm really not sure how to avoid this
            // message.

D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,  // This warning occurs when using capture frame while graphics
// debugging.

D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,  // This warning occurs when using capture frame while graphics
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


    m_DirectCommandQueue = std::unique_ptr<CommandQueue>(new CommandQueue(*this, D3D12_COMMAND_LIST_TYPE_DIRECT));
    m_ComputeCommandQueue = std::unique_ptr<CommandQueue>(new CommandQueue(*this, D3D12_COMMAND_LIST_TYPE_COMPUTE));
    m_CopyCommandQueue = std::unique_ptr<CommandQueue>(new CommandQueue(*this, D3D12_COMMAND_LIST_TYPE_COPY));

    // Create descriptor allocators
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
        m_DescriptorAllocators[i] =
            std::unique_ptr<DescriptorAllocator>(new DescriptorAllocator(*this, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i)));
    }

    // Check features.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData;
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
        if (FAILED(m_d3d12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData,
            sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE)))) {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }
        m_HighestRootSignatureVersion = featureData.HighestVersion;
    }
}

DeltaEngine::Device::~Device() {}

void Device::Flush() {
    m_DirectCommandQueue->Flush();
    m_ComputeCommandQueue->Flush();
    m_CopyCommandQueue->Flush();
}

DescriptorAllocation Device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    return m_DescriptorAllocators[type]->Allocate(numDescriptors);
}

void Device::ReleaseStaleDescriptors() {
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
        m_DescriptorAllocators[i]->ReleaseStaleDescriptors();
    }
}

void Device::ReleaseUploadResources() {
    m_InFlightUploadResources.clear();
}

CommandQueue& Device::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) {
    CommandQueue* commandQueue;
    switch (type) {
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

std::shared_ptr<DirectX12Texture> Device::CreateTexture(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue)
{
    return std::make_shared<DirectX12Texture>(*this, resourceDesc, clearValue);
}

std::shared_ptr<DirectX12Texture> Device::CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue)
{
    return std::make_shared<DirectX12Texture>(*this, resource, clearValue);
}

void Device::CreateTextureFromFile(DTexture* dtex, CommandList& commandList)
{
    if (!dtex || dtex->GetData().empty())
        return;

    auto d3d12Device = GetD3D12Device();
    UINT textureWidth = dtex->GetWidth();
    UINT textureHeight = dtex->GetHeight();
    const UINT pixelSize = 4;
    UINT64 rowPitch = textureWidth * pixelSize;
    UINT64 alignedRowPitch = (rowPitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    UINT64 textureSize = alignedRowPitch * textureHeight;

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = textureWidth;
    textureDesc.Height = textureHeight;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    ComPtr<ID3D12Resource> textureResource;
    CD3DX12_HEAP_PROPERTIES heapDefault(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
        &heapDefault,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&textureResource)));

    ComPtr<ID3D12Resource> uploadHeap;
    CD3DX12_HEAP_PROPERTIES heapUpload(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(textureSize);
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
        &heapUpload,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadHeap)));

    const unsigned char* rawData = dtex->GetData().data();
    UINT8* pData = nullptr;
    uploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    for (UINT y = 0; y < textureHeight; ++y)
        memcpy(pData + y * alignedRowPitch, rawData + y * rowPitch, rowPitch);
    uploadHeap->Unmap(0, nullptr);

    D3D12_SUBRESOURCE_FOOTPRINT pitchedDesc = {};
    pitchedDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    pitchedDesc.Width = textureWidth;
    pitchedDesc.Height = textureHeight;
    pitchedDesc.Depth = 1;
    pitchedDesc.RowPitch = static_cast<UINT>(alignedRowPitch);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed = { 0 };
    placed.Offset = 0;
    placed.Footprint = pitchedDesc;

    D3D12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(textureResource.Get(), 0);
    D3D12_TEXTURE_COPY_LOCATION src = CD3DX12_TEXTURE_COPY_LOCATION(uploadHeap.Get(), placed);
    commandList.CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    // Keep the upload heap alive until the command list is executed.
    m_InFlightUploadResources.push_back(uploadHeap);

    D3D12_RESOURCE_BARRIER rb = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList.ResourceBarrier(1, &rb);

    std::shared_ptr<DirectX12Texture> dx12Texture = std::make_shared<DirectX12Texture>(*this, textureResource, nullptr);
    dtex->SetGPUTexture(dx12Texture);
}