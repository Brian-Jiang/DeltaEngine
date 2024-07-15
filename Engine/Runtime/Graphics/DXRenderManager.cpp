#include "DXRenderManager.h"

#include <fstream>
#include <iostream>
#include <d3dcompiler.h>
#include <dxcapi.h>

#include "Graphics/DXUtils.h"
#include "IO/IOManager.h"
// #include "PlatformHelpers.h"

// using namespace DirectX;
using namespace Microsoft::WRL;
using namespace DeltaEngine;

DXRenderManager::DXRenderManager(HWND hwnd, UINT width, UINT height): hwnd(hwnd), m_width(width), m_height(height),
	m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
    m_rtvDescriptorSize(0)
{
	m_useWarpDevice = false;
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	LoadPipeline();
    LoadAssets();
}

void DXRenderManager::LoadPipeline()
{
#if defined(_DEBUG)
    // Enable the D3D12 debug layer.
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    {
	    ComPtr<ID3D12Debug> debugController;
        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
        debugController->EnableDebugLayer();
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

    auto adapter = DXUtils::GetAdapter(m_useWarpDevice);
    m_device = DXUtils::CreateDevice(adapter);

    // Describe and create the command queue.
    m_commandQueue = DXUtils::CreateCommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    // Describe and create the swap chain.
    m_swapChain = DXUtils::CreateSwapChain(hwnd, m_commandQueue, m_width, m_height, FrameCount);

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        // RTV describes the location of the texture resource in GPU memory, as well as size and format.
        // Each frame has its own RTV.
        m_rtvHeap = DXUtils::CreateDescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FrameCount);

        // Describe and create a shader resource view (SRV) heap for the texture.
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));
    }

    // Get the size of the RTV descriptor on the device. Different devices may have different sizes.
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create frame resources.
    {
        // Get a handle to the first descriptor in the descriptor heap.
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }

    // Create a command allocator, this is required for dx12.
    m_commandAllocator = DXUtils::CreateCommandAllocator(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    // ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

    g_TearingSupported = DXUtils::CheckTearingSupport();
}

void DXRenderManager::LoadAssets()
{
    // Create an empty root signature.
    // {
    //     CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    //     rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    //
    //     ComPtr<ID3DBlob> signature;
    //     ComPtr<ID3DBlob> error;
    //     ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    //     ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    // }

    // Create the root signature.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

        CD3DX12_ROOT_PARAMETER1 rootParameters[1];
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
        ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        // ComPtr<ID3DBlob> vertexShader;
        // ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif


        // Create the dxc compiler and helper interfaces.
		ComPtr<IDxcCompiler> compiler;
		ComPtr<IDxcLibrary> library;
		ComPtr<IDxcIncludeHandler> includeHandler;
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
		ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
		ThrowIfFailed(library->CreateIncludeHandler(&includeHandler));

		// Read the shader source file.
		std::wstring shaderPath = IOManager::GetAssetFullPath(L"Shaders/Shaders.hlsl");
		ComPtr<IDxcBlobEncoding> sourceBlob;
    	ThrowIfFailed(library->CreateBlobFromFile(shaderPath.c_str(), nullptr, &sourceBlob));

		// Compile the vertex shader.
		ComPtr<IDxcOperationResult> vertexShaderResult;
		ThrowIfFailed(compiler->Compile(sourceBlob.Get(), shaderPath.c_str(), L"VSMain", L"vs_6_0", nullptr, 0, nullptr, 0, includeHandler.Get(), &vertexShaderResult));

		// Check for errors.
		HRESULT hr;
		ThrowIfFailed(vertexShaderResult->GetStatus(&hr));
		if (FAILED(hr))
		{
		    ComPtr<IDxcBlobEncoding> error;
		    vertexShaderResult->GetErrorBuffer(&error);
		    // TODO: Handle the error. The error message can be retrieved from the error blob.
		}

		// Retrieve the compiled shader.
		ComPtr<IDxcBlob> vertexShader;
		vertexShaderResult->GetResult(&vertexShader);

		// Compile the pixel shader.
		// This is similar to the vertex shader compilation.
		ComPtr<IDxcOperationResult> pixelShaderResult;
		compiler->Compile(sourceBlob.Get(), shaderPath.c_str(), L"PSMain", L"ps_6_0", nullptr, 0, nullptr, 0, includeHandler.Get(), &pixelShaderResult);
		pixelShaderResult->GetStatus(&hr);
		if (FAILED(hr))
		{
		    ComPtr<IDxcBlobEncoding> error;
		    pixelShaderResult->GetErrorBuffer(&error);
		    // TODO: Handle the error.
		}
		ComPtr<IDxcBlob> pixelShader;
		pixelShaderResult->GetResult(&pixelShader);

        // ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"Shaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_6_0", compileFlags, 0, &vertexShader, nullptr));
        // ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"Shaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_6_0", compileFlags, 0, &pixelShader, nullptr));

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        D3D12_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { static_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
        psoDesc.PS = { static_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = blendDesc;
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    // Create the command list.
    // m_commandList = DXUtils::CreateCommandList(m_device, m_commandAllocator, D3D12_COMMAND_LIST_TYPE_DIRECT);
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValue = 1;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        // WaitForPreviousFrame();
    }
}

void DXRenderManager::InitFinish()
{
    // Close the command list and execute it to begin the initial GPU setup.
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    // {
    //     ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    //     m_fenceValue = 1;
    //
    //     // Create an event handle to use for frame synchronization.
    //     m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    //     if (m_fenceEvent == nullptr)
    //     {
    //         ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    //     }
    //
    //     // Wait for the command list to execute; we are reusing the same command 
    //     // list in our main loop but for now, we just want to wait for setup to 
    //     // complete before continuing.
    WaitForPreviousFrame();
    // }
}

void DXRenderManager::PrepareFrame()
{
    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    ThrowIfFailed(m_commandAllocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));

    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DXRenderManager::RenderFrame()
{
    // Let the frame transition to the render target state.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);

    // Command list needs to be closed before it can be executed. This is when the validation of the command list happens.
    ThrowIfFailed(m_commandList->Close());

    // Execute the command list.
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Present the frame.
    UINT syncInterval = g_VSync ? 1 : 0;
	UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));

    // Wait until frame is done rendering.
    WaitForPreviousFrame();
}

void DXRenderManager::WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. More advanced samples 
    // illustrate how to use fences for efficient resource usage.

    // 
    const UINT64 fence = m_fenceValue;

    // Add a signal command to the queue so that when the command queue finishes executing, it will signal the fence.
    // Then the m_fence will be signaled with the value of the fence.
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
    m_fenceValue++;

    // Wait until the previous frame is finished.
    if (m_fence->GetCompletedValue() < fence)
    {
        // Set the event to be notified when the fence is completed.
        ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));

        // Wait for the fence event to become signaled. CPU will be stalled here until the fence is signaled.
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void DXRenderManager::OnDestroy()
{
    // Wait for the GPU to be done with all resources.
    WaitForPreviousFrame();

    CloseHandle(m_fenceEvent);
}


// void DXRenderManager::GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
// {
//     *ppAdapter = nullptr;
//     for (UINT adapterIndex = 0; ; ++adapterIndex)
//     {
//         IDXGIAdapter1* pAdapter = nullptr;
//         if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter))
//         {
//             // No more adapters to enumerate.
//             break;
//         } 
//
//         // Check to see if the adapter supports Direct3D 12, but don't create the
//         // actual device yet.
//         if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
//         {
//             *ppAdapter = pAdapter;
//             return;
//         }
//         pAdapter->Release();
//     }
// }

// std::wstring DXRenderManager::GetAssetFullPath(LPCWSTR assetName)
// {
//     // std::ofstream file("relative_path_test.txt");
//     //
//     // if (file.is_open()) {
//     //     file << "Test file";
//     // }
//     // file.close();
//
//
//     std::wstring prefix = L"../../../Engine/Runtime/";
//     return prefix + assetName;
// }
