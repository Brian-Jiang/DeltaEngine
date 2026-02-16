#include "EditorRenderManager.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3dx12.h>

#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/Resource.h"
#include "Runtime/Graphics/DXUtils.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorWindows/EditorWindow_Viewport.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_dx12.h"

using namespace DeltaEngine;

#if _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

EditorRenderManager::EditorRenderManager(HWND hwnd, UINT width, UINT height)
    : m_hwnd(hwnd), m_width(width), m_height(height)
{
#ifdef DX12_ENABLE_DEBUG_LAYER
    Device::EnableDebugLayer();
#endif

    m_device = Device::Create();
    m_swapChain = m_device->CreateSwapChain(hwnd, DXGI_FORMAT_R8G8B8A8_UNORM);

    m_offscreenRenderTarget = std::make_shared<RenderTarget>();
    m_sceneRenderer = std::make_shared<DXRenderManager>(m_device, m_offscreenRenderTarget, width, height);

    m_imGuiSrvAllocator.Create(*m_device, m_device->CreateShaderVisibleSrvHeap(64));

    D3D12_CPU_DESCRIPTOR_HANDLE out_cpu;
    D3D12_GPU_DESCRIPTOR_HANDLE out_gpu;
    m_imGuiSrvAllocator.Alloc(&out_cpu, &out_gpu);
    m_imguiSrvCpuHandle = out_cpu;
    m_imguiSrvGpuHandle = out_gpu;
    m_sceneTextureId = (ImTextureID)(intptr_t)m_imguiSrvGpuHandle.ptr;
}

EditorRenderManager::~EditorRenderManager()
{
    m_imGuiSrvAllocator.Destroy();
}

const RenderTarget& EditorRenderManager::GetCurrentBackBufferRTV() const
{
    return m_swapChain->GetRenderTarget();
}

void EditorRenderManager::CopyOffscreenToBackBuffer(CommandList& commandList)
{
    auto backBuffer = m_swapChain->GetRenderTarget().GetTexture(AttachmentPoint::Color0);
    auto offscreenColor = m_offscreenRenderTarget->GetTexture(AttachmentPoint::Color0);
    if (!backBuffer || !offscreenColor)
        return;

    std::shared_ptr<Resource> srcResource = offscreenColor;
    std::shared_ptr<Resource> dstResource = backBuffer;

    if (offscreenColor->GetD3D12ResourceDesc().SampleDesc.Count > 1)
        commandList.ResolveSubresource(dstResource, srcResource);
    else
        commandList.CopyResource(dstResource, srcResource);
}

void EditorRenderManager::PrepareViewportSceneTexture(CommandList& commandList)
{
    auto offscreenColor = m_offscreenRenderTarget->GetTexture(AttachmentPoint::Color0);
    if (!offscreenColor)
        return;

    const UINT sampleCount = offscreenColor->GetD3D12ResourceDesc().SampleDesc.Count;
    const UINT width = m_offscreenRenderTarget->GetWidth();
    const UINT height = m_offscreenRenderTarget->GetHeight();

    std::shared_ptr<DirectX12Texture> displayTexture;

    if (sampleCount > 1)
    {
        // MSAA path: resolve to non-multisampled texture, then copy descriptor
        if (!m_viewportDisplayTexture ||
            m_viewportDisplayTexture->GetD3D12ResourceDesc().Width != width ||
            m_viewportDisplayTexture->GetD3D12ResourceDesc().Height != height)
        {
            DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
            auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE);
            m_viewportDisplayTexture = m_device->CreateTexture(colorDesc, nullptr);
            m_viewportDisplayTexture->SetName(L"Viewport Display Target");
        }

        commandList.ResolveSubresource(m_viewportDisplayTexture, offscreenColor);
        commandList.TransitionBarrier(m_viewportDisplayTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList.FlushResourceBarriers();
        displayTexture = m_viewportDisplayTexture;
    }
    else
    {
        // Non-MSAA path: use copy descriptor directly on the offscreen RT
        commandList.TransitionBarrier(offscreenColor, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList.FlushResourceBarriers();
        displayTexture = offscreenColor;
    }

    // Copy descriptor from display texture (on Device heap) to ImGui heap so ImGui can sample it
    D3D12_CPU_DESCRIPTOR_HANDLE srcSrv = displayTexture->GetShaderResourceView();
    if (srcSrv.ptr != 0)
    {
        m_device->GetD3D12Device()->CopyDescriptorsSimple(1, m_imguiSrvCpuHandle, srcSrv, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
}

void EditorRenderManager::RenderFrame(EngineMain* engine)
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    //ImGui::DockSpaceOverViewport();
    ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);

    m_sceneRenderer->PrepareFrame();
    engine->RecordSceneDraws(m_sceneRenderer->GetGraphicsContext());

    m_sceneRenderer->RenderFrame();
    //m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).Flush();

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = directCommandQueue.GetCommandList();

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearTexture(m_swapChain->GetRenderTarget().GetTexture(AttachmentPoint::Color0), clearColor);

    PrepareViewportSceneTexture(*commandList);

    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

    g_editor->RenderEditorWindows();

    auto backBufferRTV = m_swapChain->GetRenderTarget();
    commandList->SetRenderTarget(backBufferRTV);
    commandList->FlushResourceBarriers();
    commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_imGuiSrvAllocator.GetHeap());

    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList->GetD3D12CommandList().Get());

    directCommandQueue.ExecuteCommandList(commandList);
    m_swapChain->Present();
}

void EditorRenderManager::Resize(UINT width, UINT height)
{
    m_width = std::max(1u, width);
    m_height = std::max(1u, height);
    m_sceneRenderer->Resize(m_width, m_height);
    m_swapChain->Resize(m_width, m_height);
}

void EditorRenderManager::SetFullscreen(bool fullscreen)
{
    if (m_fullscreen != fullscreen)
    {
        m_fullscreen = fullscreen;
        if (m_fullscreen)
        {
            ::GetWindowRect(m_hwnd, &m_windowRect);
            LONG windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
            ::SetWindowLongW(m_hwnd, GWL_STYLE, windowStyle);

            HMONITOR hMonitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            GetMonitorInfo(hMonitor, &monitorInfo);

            SetWindowPos(m_hwnd, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);
            ShowWindow(m_hwnd, SW_MAXIMIZE);
        }
        else
        {
            ::SetWindowLong(m_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
            ::SetWindowPos(m_hwnd, HWND_NOTOPMOST, m_windowRect.left, m_windowRect.top,
                m_windowRect.right - m_windowRect.left, m_windowRect.bottom - m_windowRect.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);
            ::ShowWindow(m_hwnd, SW_NORMAL);
        }
    }
}

void EditorRenderManager::OnDestroy()
{
}
