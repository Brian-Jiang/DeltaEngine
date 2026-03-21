#include "EditorRenderManager.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3dx12.h>

#include "Editor/EditorMain.h"
#include "Panels/AppHeader.h"
#include "Panels/MainToolbar.h"
#include "Panels/StatusBar.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Style/EditorTheme.h"

#include "backends/imgui_impl_dx12.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui.h"

#include <algorithm>
#include <cstdint>

using namespace DeltaEngine;

#if _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

EditorRenderManager::EditorRenderManager(HWND hwnd, UINT width, UINT height)
    : m_hwnd(hwnd)
    , m_width(width)
    , m_height(height)
{
#ifdef DX12_ENABLE_DEBUG_LAYER
    Device::EnableDebugLayer();
#endif

    m_device = Device::Create();
    m_swapChain = m_device->CreateSwapChain(hwnd, DXGI_FORMAT_R8G8B8A8_UNORM);

    m_offscreenRenderTarget = std::make_shared<RenderTarget>();
    m_sceneRenderer = std::make_shared<DXRenderManager>(m_device, m_offscreenRenderTarget, width, height);

    m_imGuiSrvAllocator.Create(*m_device, m_device->CreateShaderVisibleSrvHeap(64));

    D3D12_CPU_DESCRIPTOR_HANDLE outCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE outGpu;
    m_imGuiSrvAllocator.Alloc(&outCpu, &outGpu);
    m_imguiSrvCpuHandle = outCpu;
    m_imguiSrvGpuHandle = outGpu;
    m_sceneTextureId = static_cast<ImTextureID>(m_imguiSrvGpuHandle.ptr);

    m_appHeader = std::make_unique<AppHeader>();
    m_toolbar = std::make_unique<MainToolbar>();
    m_statusBar = std::make_unique<StatusBar>();
}

EditorRenderManager::~EditorRenderManager()
{
    m_imGuiSrvAllocator.Destroy();
}

const RenderTarget& EditorRenderManager::GetCurrentBackBufferRTV() const
{
    return m_swapChain->GetRenderTarget();
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
        if (!m_viewportDisplayTexture ||
            m_viewportDisplayTexture->GetD3D12ResourceDesc().Width != width ||
            m_viewportDisplayTexture->GetD3D12ResourceDesc().Height != height)
        {
            const auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE);
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
        commandList.TransitionBarrier(offscreenColor, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList.FlushResourceBarriers();
        displayTexture = offscreenColor;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE srcSrv = displayTexture->GetShaderResourceView();
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

    m_appHeader->Draw();
    m_toolbar->Draw();

    const float topOffset = EditorTheme::HdrH() + EditorTheme::TbH();
    const float bottomOffset = EditorTheme::StH();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, topOffset));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y - topOffset - bottomOffset));
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(
        "##DockHost",
        nullptr,
        ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoDocking);
    ImGui::PopStyleVar(3);
    ImGui::DockSpace(
        ImGui::GetID("MainDockSpace"),
        ImVec2(0, 0),
        ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);
    ImGui::End();

    m_statusBar->Draw();

    m_sceneRenderer->PrepareFrame();
    engine->RecordSceneDraws(m_sceneRenderer->GetGraphicsContext());
    m_sceneRenderer->RenderFrame();

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = directCommandQueue.GetCommandList();

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearTexture(m_swapChain->GetRenderTarget().GetTexture(AttachmentPoint::Color0), clearColor);
    PrepareViewportSceneTexture(*commandList);

    g_editor->RenderEditorWindows();

    auto& backBufferRTV = m_swapChain->GetRenderTarget();
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
    m_swapChain->Resize(m_width, m_height);
}

void EditorRenderManager::SetSceneRenderSize(UINT width, UINT height)
{
    const UINT sceneWidth = std::max(1u, width);
    const UINT sceneHeight = std::max(1u, height);
    m_sceneRenderer->Resize(sceneWidth, sceneHeight);
}

void EditorRenderManager::GetSceneRenderSize(UINT& width, UINT& height) const
{
    width = m_sceneRenderer->GetWidth();
    height = m_sceneRenderer->GetHeight();
}

void EditorRenderManager::SetFullscreen(bool fullscreen)
{
    if (m_fullscreen == fullscreen)
        return;

    m_fullscreen = fullscreen;
    if (m_fullscreen)
    {
        ::GetWindowRect(m_hwnd, &m_windowRect);
        const LONG windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
        ::SetWindowLongW(m_hwnd, GWL_STYLE, windowStyle);

        const HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX monitorInfo = {};
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        GetMonitorInfo(monitor, &monitorInfo);

        SetWindowPos(
            m_hwnd,
            HWND_TOP,
            monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);
        ShowWindow(m_hwnd, SW_MAXIMIZE);
        return;
    }

    ::SetWindowLong(m_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    ::SetWindowPos(
        m_hwnd,
        HWND_NOTOPMOST,
        m_windowRect.left,
        m_windowRect.top,
        m_windowRect.right - m_windowRect.left,
        m_windowRect.bottom - m_windowRect.top,
        SWP_FRAMECHANGED | SWP_NOACTIVATE);
    ::ShowWindow(m_hwnd, SW_NORMAL);
}

void EditorRenderManager::OnDestroy()
{
}
