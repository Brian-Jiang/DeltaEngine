#include "DXRenderManager.h"

#include <fstream>
#include <iostream>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxcapi.h>
#include <dxgidebug.h>
//#include <d3dx12.h>

#include "Graphics/DXUtils.h"
#include "IO/IOManager.h"


// using namespace DirectX;
using namespace Microsoft::WRL;
using namespace DeltaEngine;
using namespace DirectX;

DXRenderManager::DXRenderManager(HWND hwnd, UINT width, UINT height): hwnd(hwnd), m_width(width), m_height(height),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
    m_rtvDescriptorSize(0)
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

    // Create the descriptor heap for the depth-stencil view.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)));

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
        //ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

        CD3DX12_ROOT_PARAMETER1 rootParameters[3];
        rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
        rootParameters[2].InitAsConstantBufferView(1);

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
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
        psoDesc.DepthStencilState.DepthEnable = FALSE;
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
    m_commandList = directCommandQueue->GetCommandList(m_pipelineState);
    
    // Set necessary state.
    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    m_commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
    m_commandList->SetGraphicsRootDescriptorTable(1, m_srvHeap->GetGPUDescriptorHandleForHeapStart());

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
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
            for (UINT n = 0; n < FrameCount; n++)
            {
                ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
                m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
                rtvHandle.Offset(1, m_rtvDescriptorSize);
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

    // Update the depth-stencil view.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    m_device->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv,
        m_DSVHeap->GetCPUDescriptorHandleForHeapStart());

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

void DXRenderManager::SetModelMatrix(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, DirectX::XMMATRIX model) {
	//auto mvp = XMMatrixMultiply(model, mvpMatrix);
    //auto mvp = mvpMatrix;
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &model, 0);
}

void DXRenderManager::ResetModelMatrix(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
}
