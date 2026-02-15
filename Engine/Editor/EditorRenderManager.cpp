#include "EditorRenderManager.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/Resource.h"
#include "Runtime/Graphics/DXUtils.h"

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

void EditorRenderManager::RenderFrame(EngineMain* engine)
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    m_sceneRenderer->PrepareFrame();
    engine->RecordSceneDraws(m_sceneRenderer->GetGraphicsContext());

    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);

    m_sceneRenderer->RenderFrame();

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = directCommandQueue.GetCommandList();

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearTexture(m_swapChain->GetRenderTarget().GetTexture(AttachmentPoint::Color0), clearColor);

    CopyOffscreenToBackBuffer(*commandList);

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
