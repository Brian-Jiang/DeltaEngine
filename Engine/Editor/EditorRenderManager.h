#pragma once

#include "EditorIncludes.h"

#include <d3d12.h>
#include <memory>
#include <optional>

#include "Runtime/Graphics/DirectX/ImGuiSrvDescriptorAllocator.h"
#include "Runtime/Graphics/DirectX/SwapChain.h"
#include "Runtime/Graphics/Structures/Camera.h"

#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class AppHeader;
class CommandList;
class Device;
class DirectX12Texture;
class DXRenderManager;
class MainToolbar;
class RenderTarget;
class StatusBar;

/// Editor-specific render manager for the main window and scene viewport.
class EditorRenderManager
{
public:
    /// Creates the editor render manager for the given HWND.
    EditorRenderManager(HWND hwnd, UINT width, UINT height);
    /// Releases render-manager owned resources.
    ~EditorRenderManager();

    /// Resizes the swap chain to match the window size.
    void Resize(UINT width, UINT height);
    /// Resizes only the scene render target.
    void SetSceneRenderSize(UINT width, UINT height);
    /// Returns the current scene render target size.
    void GetSceneRenderSize(UINT& width, UINT& height) const;
    /// Switches between windowed and fullscreen presentation.
    void SetFullscreen(bool fullscreen);
    /// Enables or disables swap-chain vsync.
    void ToggleVSync(bool enable) { m_swapChain->SetVSync(enable); }
    /// Performs any explicit shutdown work before destruction.
    void OnDestroy();

    /// Returns the shared D3D12 device wrapper.
    std::shared_ptr<Device> GetDevice() const { return m_device; }
    /// Returns the scene renderer used for offscreen rendering.
    std::shared_ptr<DXRenderManager> GetSceneRenderer() const { return m_sceneRenderer; }
    /// Returns the descriptor allocator used by ImGui textures.
    ImGuiSrvDescriptorAllocator* GetImGuiSrvAllocator() { return &m_imGuiSrvAllocator; }

    /// Returns the current back-buffer render target.
    const RenderTarget& GetCurrentBackBufferRTV() const;

    /// Renders the scene, editor UI, and presents the frame.
    void RenderFrame(class EngineMain* engine);

    /// Sets the preview / viewport camera for the next frame (shadow pass + scene draws).
    void SetActiveRenderCamera(const ActiveRenderCamera& camera) { m_activeRenderCamera = camera; }
    void ClearActiveRenderCamera() { m_activeRenderCamera.reset(); }

    /// Returns the swap-chain width in pixels.
    UINT GetWidth() const { return m_width; }
    /// Returns the swap-chain height in pixels.
    UINT GetHeight() const { return m_height; }
    /// Returns whether fullscreen mode is active.
    bool IsFullscreen() const { return m_fullscreen; }
    /// Returns whether vsync is enabled.
    bool IsVSync() const { return m_swapChain->GetVSync(); }
    /// Returns the texture shown in the viewport window.
    ImTextureID GetSceneTextureId() const { return m_sceneTextureId; }
    /// Returns the main toolbar owned by the render manager.
    MainToolbar& GetMainToolbar() { return *m_toolbar; }

private:
    /// Updates the viewport texture that ImGui samples from.
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

    std::shared_ptr<DirectX12Texture> m_viewportDisplayTexture;
    std::optional<ActiveRenderCamera> m_activeRenderCamera;

    D3D12_CPU_DESCRIPTOR_HANDLE m_imguiSrvCpuHandle;
    D3D12_GPU_DESCRIPTOR_HANDLE m_imguiSrvGpuHandle;
    ImTextureID m_sceneTextureId;

    std::unique_ptr<AppHeader> m_appHeader;
    std::unique_ptr<MainToolbar> m_toolbar;
    std::unique_ptr<StatusBar> m_statusBar;
};

DELTA_ENGINE_NS_END
