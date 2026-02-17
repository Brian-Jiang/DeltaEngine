#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <d3d12.h>

#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/SwapChain.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/ImGuiSrvDescriptorAllocator.h"

#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class DXRenderManager;
class CommandList;
class EditorWindow_WorldOutliner;
class EditorWindow_Viewport;

/// Editor-specific render manager. Owns window, swap chain, offscreen RT for scene, and ImGui.
/// Resembles DXRenderManager structure but targets the actual window.
class EditorRenderManager
{
public:
    EditorRenderManager(HWND hwnd, UINT width, UINT height);
    ~EditorRenderManager();

    void Resize(UINT width, UINT height);
    /// Resize only the scene render target (not swap chain). Called by viewport when settings change.
    void SetSceneRenderSize(UINT width, UINT height);
    void GetSceneRenderSize(UINT& width, UINT& height) const;
    void SetFullscreen(bool fullscreen);
    void ToggleVSync(bool enable) { m_swapChain->SetVSync(enable); }
    void OnDestroy();

    std::shared_ptr<Device> GetDevice() const { return m_device; }
    std::shared_ptr<DXRenderManager> GetSceneRenderer() const { return m_sceneRenderer; }
    ImGuiSrvDescriptorAllocator* GetImGuiSrvAllocator() { return &m_imGuiSrvAllocator; }

    /// Returns the current back buffer render target for ImGui.
    const RenderTarget& GetCurrentBackBufferRTV() const;

    /// Renders scene to offscreen, copies to backbuffer, renders ImGui, presents.
    void RenderFrame(class EngineMain* engine);

    UINT GetWidth() const { return m_width; }
    UINT GetHeight() const { return m_height; }
    bool IsFullscreen() const { return m_fullscreen; }
    bool IsVSync() const { return m_swapChain->GetVSync(); }
    ImTextureID GetSceneTextureId() const { return m_sceneTextureId; }

private:
    void CopyOffscreenToBackBuffer(CommandList& commandList);

    /// Prepares the scene texture for viewport display. Uses copy descriptor when RT is non-MSAA,
    /// or blits to m_viewportDisplayTexture and copies its descriptor when MSAA.
    void PrepareViewportSceneTexture(CommandList& commandList);

    HWND m_hwnd;
    RECT m_windowRect;
    bool m_fullscreen = false;
    UINT m_width;
    UINT m_height;

    std::shared_ptr<Device> m_device;
    std::shared_ptr<SwapChain> m_swapChain;
    std::shared_ptr<RenderTarget> m_offscreenRenderTarget;
    std::shared_ptr<DXRenderManager> m_sceneRenderer;
    ImGuiSrvDescriptorAllocator m_imGuiSrvAllocator;

    /// Texture for blit path when offscreen RT is multisampled. Resolves MSAA RT to this for ImGui display.
    std::shared_ptr<DirectX12Texture> m_viewportDisplayTexture;

    D3D12_CPU_DESCRIPTOR_HANDLE m_imguiSrvCpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_imguiSrvGpuHandle;
    ImTextureID m_sceneTextureId;
};

DELTA_ENGINE_NS_END
