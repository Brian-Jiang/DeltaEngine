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
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/DirectX/SwapChain.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/Adapter.h"
#include "Runtime/Core/DWorld.h"

using namespace Microsoft::WRL;
using namespace DeltaEngine;
using namespace DirectX;

DXRenderManager::DXRenderManager(HWND hwnd, UINT width, UINT height)
    : hwnd(hwnd), m_width(width), m_height(height), g_Fullscreen(false),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
{
#if _DEBUG
    Device::EnableDebugLayer();
#endif

    // Check for DirectX Math library support.
    if (!DirectX::XMVerifyCPUSupport())
    {
        MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
    }

    //m_useWarpDevice = false;
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    LoadPipeline();
    LoadAssets();
}

void DXRenderManager::LoadPipeline()
{
    m_device = Device::Create();
    m_swapChain = m_device->CreateSwapChain(hwnd, DXGI_FORMAT_R8G8B8A8_UNORM);
    m_renderTarget = std::make_shared<RenderTarget>();
}

void DXRenderManager::LoadAssets()
{
    // todo root signature should bind to pass?
    // ---- Root signature (shared across all renderers) ----

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_DESCRIPTOR_RANGE1 ranges[1] {};
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 rootParameters[4] {};
    // Camera
    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
    // Texture
    rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
    // Object
    rootParameters[2].InitAsConstantBufferView(1);
    // Light
    rootParameters[3].InitAsConstantBufferView(2);

    CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(4, rootParameters, 1, &anisotropicSampler, rootSignatureFlags);

    m_rootSignature = m_device->CreateRootSignature(rootSignatureDescription.Desc_1_1);
}

void DXRenderManager::InitWorldRenderers(DWorld& world)
{
    // Create a color buffer with sRGB for gamma correction.
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

    // Check the best multisample quality level that can be used for the given back buffer format.
    DXGI_SAMPLE_DESC sampleDesc = m_device->GetMultisampleQualityLevels(backBufferFormat);

    // Create an off-screen render target with a single color buffer and a depth buffer.
    auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, m_width, m_height, 1, 1, sampleDesc.Count,
        sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    D3D12_CLEAR_VALUE colorClearValue;
    colorClearValue.Format = colorDesc.Format;
    colorClearValue.Color[0] = 0.0f;
    colorClearValue.Color[1] = 0.2f;
    colorClearValue.Color[2] = 0.4f;
    colorClearValue.Color[3] = 1.0f;

    auto colorTexture = m_device->CreateTexture(colorDesc, &colorClearValue);
    colorTexture->SetName(L"Color Render Target");

    // Create a depth buffer.
    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_width, m_height, 1, 1, sampleDesc.Count,
        sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE depthClearValue;
    depthClearValue.Format = depthDesc.Format;
    depthClearValue.DepthStencil = { 1.0f, 0 };

    auto depthTexture = m_device->CreateTexture(depthDesc, &depthClearValue);
    depthTexture->SetName(L"Depth Render Target");

    m_renderTarget->AttachTexture(AttachmentPoint::Color0, colorTexture);
    m_renderTarget->AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

    auto context = GetGraphicsContext();
    world.InitRenderers(context);
}

void DXRenderManager::PrepareFrame()
{
    m_device->ReleaseStaleDescriptors();

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = directCommandQueue.GetCommandList();
    m_currentCommandList = commandList;

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearTexture(m_renderTarget->GetTexture(AttachmentPoint::Color0), clearColor);
    commandList->ClearDepthStencilTexture(m_renderTarget->GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);

    commandList->SetViewport(m_viewport);
    commandList->SetScissorRect(m_scissorRect);
    commandList->SetRenderTarget(*m_renderTarget);
    commandList->SetGraphicsRootSignature(m_rootSignature);
}

void DXRenderManager::RenderFrame()
{
    auto swapChainBackBuffer = m_swapChain->GetRenderTarget().GetTexture(AttachmentPoint::Color0);
    auto msaaRenderTarget = m_renderTarget->GetTexture(AttachmentPoint::Color0);
    m_currentCommandList->ResolveSubresource(swapChainBackBuffer, msaaRenderTarget);

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

    directCommandQueue.ExecuteCommandList(m_currentCommandList);
    m_currentCommandList = nullptr;
    m_swapChain->Present();
}

void DXRenderManager::Resize(UINT width, UINT height)
{
    m_width = std::max(1u, width);
    m_height = std::max(1u, height);

    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
    m_renderTarget->Resize(m_width, m_height);

    m_swapChain->Resize(width, height);
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

void DXRenderManager::OnDestroy()
{

}

std::shared_ptr<DXGraphicsContext> DeltaEngine::DXRenderManager::GetGraphicsContext()
{
    auto context = std::make_shared<DXGraphicsContext>();
    context->renderManager = shared_from_this();
    context->device = m_device;
    context->commandList = m_currentCommandList;

    return context;
}
