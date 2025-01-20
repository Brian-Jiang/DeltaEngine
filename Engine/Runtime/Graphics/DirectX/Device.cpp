#include "Runtime/Graphics/DirectX/Device.h"

#include <assert.h>
#include <dxgidebug.h>
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include "Runtime/Graphics/DirectX/DescriptorAllocation.h"
#include "Runtime/Graphics/DirectX/DescriptorAllocator.h"
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

DeltaEngine::Device::Device(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter)
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


    m_DirectCommandQueue = std::make_unique<CommandQueue>(this->GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_ComputeCommandQueue = std::make_unique<CommandQueue>(this->GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_CopyCommandQueue = std::make_unique<CommandQueue>(this->GetD3D12Device(), D3D12_COMMAND_LIST_TYPE_COPY);

    // Create descriptor allocators
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
        m_DescriptorAllocators[i] =
            std::make_unique<DescriptorAllocator>(this->GetD3D12Device(), static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
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

DescriptorAllocation Device::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors) {
    return m_DescriptorAllocators[type]->Allocate(numDescriptors);
}

void Device::ReleaseStaleDescriptors() {
    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
        m_DescriptorAllocators[i]->ReleaseStaleDescriptors();
    }
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

std::shared_ptr<Texture> Device::CreateTexture(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue) {
    std::shared_ptr<Texture> texture = std::make_shared<Texture>(*this, resourceDesc, clearValue);

    return texture;
}

std::shared_ptr<Texture> Device::CreateTexture(Microsoft::WRL::ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue) {
    std::shared_ptr<Texture> texture = std::make_shared<Texture>(*this, resource, clearValue);

    return texture;
}