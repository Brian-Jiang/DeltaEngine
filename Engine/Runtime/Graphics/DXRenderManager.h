#pragma once

#include "Runtime/EngineIncludes.h"

#include <chrono>
#include <memory>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMath.h>

#include "DirectX/CommandQueue.h"
#include "DirectX/UploadBuffer.h"
#include "DirectX/DescriptorAllocator.h"
#include "Structures/Light.h"
#include "Structures/Camera.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/SwapChain.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;
class RootSignature;
class SwapChain;
class RenderTarget;
class DWorld;

class DXRenderManager : public std::enable_shared_from_this<DXRenderManager>
{
public:
	DXRenderManager(HWND hwnd, UINT width, UINT height);
	void LoadPipeline();
    void LoadAssets();

    /// Creates a command list, initialises every renderer in the world,
    /// executes the command list, and waits for the GPU.
    void InitWorldRenderers(DWorld& world);

    void PrepareFrame();
    void RenderFrame();

	void Resize(UINT width, UINT height);
	void SetFullscreen(bool fullscreen);
    void OnDestroy();

	/// Returns a context that renderers use for InitGraphicState / GatherDrawCalls.
    std::shared_ptr<DXGraphicsContext> GetGraphicsContext();

	void ToggleVSync(bool enableVSync) { m_swapChain->SetVSync(enableVSync); }

	bool IsFullscreen() { return g_Fullscreen; }
	bool IsVSync() { return m_swapChain->GetVSync(); }

	inline UINT GetWidth() const { return m_width; }
    inline UINT GetHeight() const { return m_height; }
    inline float GetAspectRatio() const { return m_aspectRatio; }

	inline std::shared_ptr<RootSignature> GetRootSignature() const { return m_rootSignature; }

private:
	HWND hwnd;

	// Window rectangle (used to toggle fullscreen state).
	RECT g_WindowRect;

	// By default, use windowed mode.
	// Can be toggled with the Alt+Enter or F11
	bool g_Fullscreen = false;

    //bool m_useWarpDevice;

	D3D12_RECT m_scissorRect;
	CD3DX12_VIEWPORT m_viewport;

    std::shared_ptr<SwapChain> m_swapChain;
	std::shared_ptr<Device> m_device;
    std::shared_ptr<RenderTarget> m_renderTarget;

    std::shared_ptr<RootSignature> m_rootSignature;

	/// The command list for the current frame, obtained in PrepareFrame
	/// and executed in RenderFrame.
	std::shared_ptr<CommandList> m_currentCommandList;

    UINT m_width;
    UINT m_height;
    float m_aspectRatio;

	//DirectX::XMFLOAT4X4 m_viewMatrix;
	//DirectX::XMFLOAT4X4 m_projectionMatrix;
	//DirectX::XMFLOAT4 m_cameraPosition;
};

DELTA_ENGINE_NS_END
