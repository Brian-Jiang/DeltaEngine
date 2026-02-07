#include "DXRenderManager.h"

#include <fstream>
#include <iostream>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgidebug.h>

#include "Graphics/DXUtils.h"
#include "IO/IOManager.h"
#include "Core/Time.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Core/DWorld.h"

using namespace Microsoft::WRL;
using namespace DeltaEngine;
using namespace DirectX;

DXRenderManager::DXRenderManager(HWND hwnd, UINT width, UINT height)
    : hwnd(hwnd), m_width(width), m_height(height),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)),
    m_rtvDescriptorSize(0)
{

    // Check for DirectX Math library support.
    if (!DirectX::XMVerifyCPUSupport())
    {
        MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
    }

    m_useWarpDevice = false;
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    LoadPipeline();
    LoadAssets();
}

void DXRenderManager::LoadPipeline()
{
#if defined(_DEBUG)
    {
        Device::EnableDebugLayer();
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));

    auto adapter = DXUtils::GetAdapter(m_useWarpDevice);
    m_device = std::make_shared<Device>(adapter);

    // Initialize descriptor allocators after device is created
    m_rtvHeap = std::unique_ptr<DescriptorAllocator>(new DescriptorAllocator(*m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    m_DSVHeap = std::unique_ptr<DescriptorAllocator>(new DescriptorAllocator(*m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV));

    // Initialize the upload buffer
    m_uploadBuffer = std::make_unique<UploadBuffer>(*m_device);

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

    // Describe and create the swap chain.
    m_swapChain = DXUtils::CreateSwapChain(hwnd, directCommandQueue.GetD3D12CommandQueue(), m_width, m_height, FrameCount);

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a shader resource view (SRV) heap for the texture.
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 30;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ThrowIfFailed(m_device->GetD3D12Device()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap)));
    }

    // Get the size of the RTV descriptor on the device.
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    m_DSVHeapAllocation = m_DSVHeap->Allocate(1);

    // Create frame resources.
    {
        m_rtvHeapAllocation = m_rtvHeap->Allocate(FrameCount);

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->GetD3D12Device()->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, m_rtvHeapAllocation.GetDescriptorHandle(n));
        }
    }

    g_TearingSupported = DXUtils::CheckTearingSupport();
}

void DXRenderManager::LoadAssets()
{
    // ---- Light constant buffer (stays in DXRenderManager) ----
    m_light.position = XMFLOAT3(0.0f, 5.0f, 3.0f);
    m_light.intensity = 1.0f;
    m_light.color = XMFLOAT3(1.0f, 1.0f, 1.0f);

    {
        UINT alignedBufferSize = (sizeof(Light) + 255) & ~255;

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

        HRESULT hr = m_device->GetD3D12Device()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_lightCbData)
        );

        Light* mappedLightBuffer = nullptr;
        m_lightCbData->Map(0, nullptr, reinterpret_cast<void**>(&mappedLightBuffer));
        memcpy(mappedLightBuffer, &m_light, sizeof(Light));
        m_lightCbData->Unmap(0, nullptr);
    }

    // ---- Root signature (shared across all renderers) ----
    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

        if (FAILED(m_device->GetD3D12Device()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
        {
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
        }

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1]{};
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

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
        ThrowIfFailed(m_device->GetD3D12Device()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
    }

    // NOTE: PSO creation and shader compilation have been moved to
    // individual Renderer::InitGraphicState() implementations.

    // Resize/Create the depth buffer.
    ResizeDepthBuffer(m_width, m_height);

    // Create synchronization objects.
    {
        for (UINT n = 0; n < FrameCount; n++)
        {
            frameFenceValues[n] = 0;
        }
    }
}

void DXRenderManager::InitWorldRenderers(DWorld& world)
{
    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_currentCommandList = directCommandQueue.GetCommandList();

    DXGraphicsContext context = GetGraphicsContext();
    world.InitRenderers(context);

    frameFenceValues[m_frameIndex] = directCommandQueue.ExecuteCommandList(m_currentCommandList);
    m_currentCommandList.reset();

    WaitForPreviousFrame();
}

void DXRenderManager::PrepareFrame()
{
    m_device->ReleaseStaleDescriptors();

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_currentCommandList = directCommandQueue.GetCommandList();

    // Indicate that the back buffer will be used as a render target.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_currentCommandList->ResourceBarrier(1, &barrier);

    // Set necessary state.
    m_currentCommandList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
    m_currentCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    m_currentCommandList->SetGraphicsRootConstantBufferView(3, m_lightCbData->GetGPUVirtualAddress());

    m_currentCommandList->RSSetViewports(1, &m_viewport);
    m_currentCommandList->RSSetScissorRects(1, &m_scissorRect);

    auto rtvHandle = m_rtvHeapAllocation.GetDescriptorHandle(m_frameIndex);
    auto dsvHandle = m_DSVHeapAllocation.GetDescriptorHandle(0);
    m_currentCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_currentCommandList->ClearRenderTargetView(rtvHandle, clearColor);
    m_currentCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0);
    m_currentCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void DXRenderManager::RenderFrame()
{
    // Transition back buffer to present state.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_currentCommandList->ResourceBarrier(1, &barrier);

    // Execute the *same* command list that PrepareFrame opened.
    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    frameFenceValues[m_frameIndex] = directCommandQueue.ExecuteCommandList(m_currentCommandList);
    m_currentCommandList.reset();

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
    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    directCommandQueue.WaitForFenceValue(frameFenceValues[m_frameIndex]);

    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void DXRenderManager::Resize(UINT width, UINT height)
{
    if (m_width != width || m_height != height)
    {
        //CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
        //directCommandQueue.Flush();
        m_device->Flush();

        // Release the resources holding references to the swap chain.
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
            for (UINT n = 0; n < FrameCount; n++)
            {
                ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
                m_device->GetD3D12Device()->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, m_rtvHeapAllocation.GetDescriptorHandle(n));
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
            ::GetWindowRect(hwnd, &g_WindowRect);

            LONG windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
            ::SetWindowLongW(hwnd, GWL_STYLE, windowStyle);

            HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            GetMonitorInfo(hMonitor, &monitorInfo);

            SetWindowPos(hwnd, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ShowWindow(hwnd, SW_MAXIMIZE);
        }
        else
        {
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
    WaitForPreviousFrame();
    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    directCommandQueue.Flush();

	width = std::max(1, width);
	height = std::max(1, height);

    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = { 1.0f, 0 };

    auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto rd = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ThrowIfFailed(m_device->GetD3D12Device()->CreateCommittedResource(
        &hp,
        D3D12_HEAP_FLAG_NONE,
        &rd,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&m_DepthBuffer)
    ));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    auto dsvHandle = m_DSVHeapAllocation.GetDescriptorHandle(0);
    m_device->GetD3D12Device()->CreateDepthStencilView(m_DepthBuffer.Get(), &dsv, dsvHandle);
}

void DXRenderManager::OnDestroy()
{
    WaitForPreviousFrame();
}

DXGraphicsContext DeltaEngine::DXRenderManager::GetGraphicsContext() const {
    DXGraphicsContext context;
    context.device = m_device.get();
    context.commandList = m_currentCommandList;
    context.rootSignature = m_rootSignature;
    context.srvHeap = m_srvHeap;
    return context;
}

CommandQueue& DXRenderManager::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const
{
    return m_device->GetCommandQueue(type);
}

UINT DeltaEngine::DXRenderManager::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const {
    return m_device->GetDescriptorHandleIncrementSize(type);
}
