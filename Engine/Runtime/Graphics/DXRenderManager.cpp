#include "DXRenderManager.h"

#include <fstream>
#include <iostream>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxcapi.h>
#include <dxgidebug.h>

#include "Graphics/DXUtils.h"
#include "IO/IOManager.h"
#include "Core/Time.h"

using namespace Microsoft::WRL;
using namespace DeltaEngine;
using namespace DirectX;

DXRenderManager::DXRenderManager(HWND hwnd, UINT width, UINT height): hwnd(hwnd), m_width(width), m_height(height),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
    m_rtvDescriptorSize(0), m_dxUploadBuffer(4 * 1024 * 1024), m_DSVHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV),
    m_rtvHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
{

    // Check for DirectX Math library support.
    if (!DirectX::XMVerifyCPUSupport())
    {
        MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
        // return false;
    }

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

    m_dxUploadBuffer.SetDevice(m_device);
    // Create command queues.
    directCommandQueue = new DXCommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    copyCommandQueue = new DXCommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_COPY);

    // Describe and create the swap chain.
    m_swapChain = DXUtils::CreateSwapChain(hwnd, directCommandQueue->GetCommandQueue(), m_width, m_height, FrameCount);

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        // RTV describes the location of the texture resource in GPU memory, as well as size and format.
        // Each frame has its own RTV.
        //m_rtvHeap = DXUtils::CreateDescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FrameCount);
        m_rtvHeap.SetDevice(m_device);

        // Describe and create a shader resource view (SRV) heap for the texture.
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 30;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));
    }

    // Get the size of the RTV descriptor on the device. Different devices may have different sizes.
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create the descriptor heap for the depth-stencil view.
    //D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    //dsvHeapDesc.NumDescriptors = 1;
    //dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    //dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    //ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));

    m_DSVHeap.SetDevice(m_device);
    m_DSVHeapAllocation = m_DSVHeap.Allocate(1);

    // Create frame resources.
    {
        m_rtvHeapAllocation = m_rtvHeap.Allocate(FrameCount);
        // Get a handle to the first descriptor in the descriptor heap.
        //CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
        //auto rtvHandle = m_rtvHeapAllocation.GetDescriptorHandle(0);

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, m_rtvHeapAllocation.GetDescriptorHandle(n));
            //rtvHandle.Offset(1, m_rtvDescriptorSize);
        }
    }

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

    m_light.position = XMFLOAT3(0.0f, 5.0f, 3.0f);
    m_light.intensity = 1.0f;
    m_light.color = XMFLOAT3(1.0f, 1.0f, 1.0f);

    {
        UINT alignedBufferSize = (sizeof(Light) + 255) & ~255;
        //auto addrPair = m_dxUploadBuffer.Allocate(sizeof(Light), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        //memcpy(addrPair.m_cpuAddr, &m_light, sizeof(Light));
        ////D3D12_CONSTANT_BUFFER_VIEW_DESC

        //// TODO transition
        //D3D12_RESOURCE_DESC resourceDesc = {};
        //resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        //resourceDesc.Width = alignedBufferSize;
        //resourceDesc.Height = 1;
        //resourceDesc.DepthOrArraySize = 1;
        //resourceDesc.MipLevels = 1;
        //resourceDesc.SampleDesc.Count = 1;
        //resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        //resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        //auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        ////ComPtr<ID3D12Resource> textureResource;
        //ThrowIfFailed(m_device->CreateCommittedResource(
        //    &hp,
        //    D3D12_HEAP_FLAG_NONE,
        //    &resourceDesc,
        //    D3D12_RESOURCE_STATE_GENERIC_READ,
        //    nullptr,
            //IID_PPV_ARGS(&m_lightCbData)));


        //ComPtr<ID3D12Resource> constantBufferResource;
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc = {};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = alignedBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_lightCbData)
        );

        Light* mappedLightBuffer = nullptr;
        m_lightCbData->Map(0, nullptr, reinterpret_cast<void**>(&mappedLightBuffer));

        // Copy the data into the mapped constant buffer.
        memcpy(mappedLightBuffer, &m_light, sizeof(Light));

        m_lightCbData->Unmap(0, nullptr);
    }

    // Create the root signature.
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1]{};
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
        //ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

        CD3DX12_ROOT_PARAMETER1 rootParameters[4]{};
        // Camera
        rootParameters[0].InitAsConstantBufferView(0);
        // Texture
        rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        // Object
        rootParameters[2].InitAsConstantBufferView(1);
        // Light
        rootParameters[3].InitAsConstantBufferView(2);

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
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
//#if defined(_DEBUG)
//        // Enable better shader debugging with the graphics debugging tools.
//        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
//#else
//        UINT compileFlags = 0;
//#endif


        // Create the dxc compiler and helper interfaces.
        ComPtr<IDxcUtils> dxcUtils;
        ComPtr<IDxcCompiler3> compiler;
        ComPtr<IDxcIncludeHandler> includeHandler;
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));
        ThrowIfFailed(dxcUtils->CreateDefaultIncludeHandler(&includeHandler));

        // Read the shader source file.
        std::wstring shaderPath = IOManager::GetAssetFullPath(L"Shaders/Shaders.hlsl");
        ComPtr<IDxcBlobEncoding> sourceBlob;
        ThrowIfFailed(dxcUtils->LoadFile(shaderPath.c_str(), nullptr, &sourceBlob));
        
        ComPtr<IDxcCompilerArgs> arguments;
        ThrowIfFailed(dxcUtils->BuildArguments(shaderPath.c_str(), L"VSMain", L"vs_6_0", nullptr, 0, nullptr, 0, &arguments));
        
        BOOL known;
        UINT32 encoding;
        ThrowIfFailed(sourceBlob->GetEncoding(&known, &encoding));
        DxcBuffer sourceBuffer{ .Ptr = sourceBlob->GetBufferPointer(), .Size = sourceBlob->GetBufferSize(), .Encoding = encoding };

        // Compile the vertex shader.
        ComPtr<IDxcResult> vertexShaderResult;
        ThrowIfFailed(compiler->Compile(&sourceBuffer, arguments->GetArguments(), arguments->GetCount(), includeHandler.Get(), IID_PPV_ARGS(&vertexShaderResult)));

        // Check for errors.
        HRESULT hr;
        ThrowIfFailed(vertexShaderResult->GetStatus(&hr));
        if (FAILED(hr))
        {
            ComPtr<IDxcBlobEncoding> error;
            vertexShaderResult->GetErrorBuffer(&error);
            // TODO: Handle the error. The error message can be retrieved from the error blob.
            std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
            std::cerr << errorMessage << std::endl;
        }

        // Retrieve the compiled shader.
        ComPtr<IDxcBlob> vertexShader;
        vertexShaderResult->GetResult(&vertexShader);

        // Compile the pixel shader.
        // This is similar to the vertex shader compilation.
        ThrowIfFailed(dxcUtils->BuildArguments(shaderPath.c_str(), L"PSMain", L"ps_6_0", nullptr, 0, nullptr, 0, &arguments));
        ComPtr<IDxcResult> pixelShaderResult;
        compiler->Compile(&sourceBuffer, arguments->GetArguments(), arguments->GetCount(), includeHandler.Get(), IID_PPV_ARGS(&pixelShaderResult));
        pixelShaderResult->GetStatus(&hr);
        if (FAILED(hr))
        {
            ComPtr<IDxcBlobEncoding> error;
            pixelShaderResult->GetErrorBuffer(&error);
            // TODO: Handle the error.
            std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
            std::cerr << errorMessage << std::endl;
        }

        ComPtr<IDxcBlob> pixelShader;
        pixelShaderResult->GetResult(&pixelShader);

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

            // Instance data (per-instance, unique to each instance)
            { "INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        };

        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
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
        psoDesc.DepthStencilState.DepthEnable = TRUE;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    // Get the command list.
    m_commandList = directCommandQueue->GetCommandList(m_pipelineState);

    // Resize/Create the depth buffer.
    ResizeDepthBuffer(m_width, m_height);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        for (UINT n = 0; n < FrameCount; n++)
        {
            frameFenceValues[n] = 0;
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
    frameFenceValues[m_frameIndex] = directCommandQueue->ExecuteCommandList(m_commandList);

    WaitForPreviousFrame();
}

void DXRenderManager::PrepareFrame()
{
    m_rtvHeap.ReleaseAllStale(Time::frameSinceStart);
    m_DSVHeap.ReleaseAllStale(Time::frameSinceStart);

    m_commandList = directCommandQueue->GetCommandList(m_pipelineState);
    
    // Indicate that the back buffer will be used as a render target.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    //m_commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
    //m_commandList->SetGraphicsRoot32BitConstants(3, sizeof(XMFLOAT4) / 4, &cameraPosition, 0);
    //m_commandList->SetGraphicsRootDescriptorTable(1, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
    m_commandList->SetGraphicsRootConstantBufferView(3, m_lightCbData->GetGPUVirtualAddress());

    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    //CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    auto rtvHandle = m_rtvHeapAllocation.GetDescriptorHandle(m_frameIndex);
    //D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
    auto dsvHandle = m_DSVHeapAllocation.GetDescriptorHandle(0);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    
}

void DXRenderManager::RenderFrame()
{
    // Let the frame transition to the render target state.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);

    frameFenceValues[m_frameIndex] = directCommandQueue->ExecuteCommandList(m_commandList);

    // Present the frame.
    UINT syncInterval = g_VSync ? 1 : 0;
    UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));

    // Wait until frame is done rendering.
    WaitForPreviousFrame();
}

void DXRenderManager::WaitForPreviousFrame()
{
    // Wait until the previous frame is finished.
    directCommandQueue->WaitForFenceValue(frameFenceValues[m_frameIndex]);

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void DXRenderManager::Resize(UINT width, UINT height)
{
    if (m_width != width || m_height != height)
    {
        // Wait for the GPU to be done with all resources.
        // WaitForPreviousFrame();
        directCommandQueue->Flush();

        // Release the resources holding references to the swap chain (requirement of IDXGISwapChain::ResizeBuffers).
        for (UINT n = 0; n < FrameCount; n++)
        {
            m_renderTargets[n].Reset();
        }

        m_width = std::max(1u, width);
        m_height = std::max(1u, height);

        // Resize the swap chain to the desired dimensions.
        DXGI_SWAP_CHAIN_DESC desc = {};
        ThrowIfFailed(m_swapChain->GetDesc(&desc));
        ThrowIfFailed(m_swapChain->ResizeBuffers(FrameCount, m_width, m_height, desc.BufferDesc.Format, desc.Flags));

        m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));

		ResizeDepthBuffer(m_width, m_height);
        
        m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        // Create frame resources.
        {
            //CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
            for (UINT n = 0; n < FrameCount; n++)
            {
                ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
                m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, m_rtvHeapAllocation.GetDescriptorHandle(n));
                //rtvHandle.Offset(1, m_rtvDescriptorSize);
            }
        }
    }
}

void DXRenderManager::SetFullscreen(bool fullscreen)
{
    if (g_Fullscreen != fullscreen)
    {
        g_Fullscreen = fullscreen;
        if (g_Fullscreen)
        {
            // Store the current window dimensions so they can be restored 
            // when switching out of fullscreen state.
            ::GetWindowRect(hwnd, &g_WindowRect);

            // Set the window style to a borderless window so the client area fills the entire screen.
            LONG windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
 
            ::SetWindowLongW(hwnd, GWL_STYLE, windowStyle);

            // Query the name of the nearest display device for the window.
            HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            GetMonitorInfo(hMonitor, &monitorInfo);

            // Change the display settings to use the closest display device.
            // DEVMODE devMode = {};
            // devMode.dmSize = sizeof(DEVMODE);
            // devMode.dmPelsWidth = m_width;
            // devMode.dmPelsHeight = m_height;
            // devMode.dmBitsPerPel = 32;
            // devMode.dmDisplayFrequency = 60;
            // devMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
            // ChangeDisplaySettings(&devMode, CDS_FULLSCREEN);

            // Set the window's size and position to cover the entire screen.
            SetWindowPos(hwnd, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);

            // Show the window in full screen.
            ShowWindow(hwnd, SW_MAXIMIZE);
        }
        else
        {
            // Restore all the window decorators.
            ::SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
 
            ::SetWindowPos(hwnd, HWND_NOTOPMOST,
                g_WindowRect.left,
                g_WindowRect.top,
                g_WindowRect.right - g_WindowRect.left,
                g_WindowRect.bottom - g_WindowRect.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);
 
            ::ShowWindow(hwnd, SW_NORMAL);
        }
    }
}

void DXRenderManager::ResizeDepthBuffer(int width, int height) {
	// Wait for the GPU to be done with all resources.
	WaitForPreviousFrame();
	directCommandQueue->Flush();

	// Release the resources holding references to the swap chain (requirement of IDXGISwapChain::ResizeBuffers).
	//for (UINT n = 0; n < FrameCount; n++) {
	//	m_renderTargets[n].Reset();
	//}

	width = std::max(1, width);
	height = std::max(1, height);

    // Resize screen dependent resources.
        // Create a depth buffer.
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 1.0f, 0 };

    auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto rd = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ThrowIfFailed(m_device->CreateCommittedResource(
        &hp,
        D3D12_HEAP_FLAG_NONE,
        &rd,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&m_DepthBuffer)
    ));
    //m_DSVHeap->SetName(L"Depth/Stencil Resource Heap");

    // Update the depth-stencil view.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    auto dsvHandle = m_DSVHeapAllocation.GetDescriptorHandle(0);
    m_device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv, dsvHandle);

	// Resize the swap chain to the desired dimensions.
	//DXGI_SWAP_CHAIN_DESC desc = {};
	//ThrowIfFailed(m_swapChain->GetDesc(&desc));
	//ThrowIfFailed(m_swapChain->ResizeBuffers(FrameCount, m_width, m_height, desc.BufferDesc.Format, desc.Flags));

	//m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));

	//m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	//m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	//// Create frame resources.
	//{
	//	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	//	for (UINT n = 0; n < FrameCount; n++) {
	//		ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
	//		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
	//		rtvHandle.Offset(1, m_rtvDescriptorSize);
	//	}
	//}
}

void DXRenderManager::OnDestroy()
{
    // Wait for the GPU to be done with all resources.
    WaitForPreviousFrame();
}

DXCommandQueue* DXRenderManager::GetCommandQueue(const D3D12_COMMAND_LIST_TYPE type) const
{
    switch (type)
    {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            return directCommandQueue;
        case D3D12_COMMAND_LIST_TYPE_COPY:
            return copyCommandQueue;
        default:
            return nullptr;
    }
}
