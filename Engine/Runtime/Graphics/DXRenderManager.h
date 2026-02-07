#pragma once

#include "Runtime/EngineIncludes.h"

#include <chrono>
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

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;
class RootSignature;
class SwapChain;
class DWorld;

class DXRenderManager
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
    void WaitForPreviousFrame();
	void Resize(UINT width, UINT height);
	void SetFullscreen(bool fullscreen);
	void ResizeDepthBuffer(int width, int height);
    void OnDestroy();

	/// Returns a context that renderers use for InitGraphicState / GatherDrawCalls.
	DXGraphicsContext GetGraphicsContext() const;

	CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;
    UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

	UploadBuffer& GetUploadBuffer() { return *m_uploadBuffer; }

	void ToggleVSync(bool enableVSync) { g_VSync = enableVSync; }

	Device& GetDeviceRef() { return *m_device; }
	Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice() { return m_device->GetD3D12Device(); }
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSRVHeap() { return m_srvHeap; }
	bool IsFullscreen() { return g_Fullscreen; }
	bool IsVSync() { return g_VSync; }

	UINT GetWidth() { return m_width; }
	UINT GetHeight() { return m_height; }

	void SetCameraPosition(DirectX::XMVECTOR cam) { DirectX::XMStoreFloat4(&m_cameraPosition, cam); }
	void SetViewMatrix(DirectX::XMMATRIX view) { DirectX::XMStoreFloat4x4(&m_viewMatrix, view); }
	void SetProjectionMatrix(DirectX::XMMATRIX proj) { DirectX::XMStoreFloat4x4(&m_projectionMatrix, proj); }

private:
	HWND hwnd;

	// Window rectangle (used to toggle fullscreen state).
	RECT g_WindowRect;

    // By default, enable V-Sync.
	// Can be toggled with the V key.
	bool g_VSync = true;

	bool g_TearingSupported = false;

	// By default, use windowed mode.
	// Can be toggled with the Alt+Enter or F11
	bool g_Fullscreen = false;

    static const UINT FrameCount = 2;

    bool m_useWarpDevice;

	CD3DX12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;

	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	std::shared_ptr<Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;

	/// The command list for the current frame, obtained in PrepareFrame
	/// and executed in RenderFrame.
	std::shared_ptr<CommandList> m_currentCommandList;

    UINT m_rtvDescriptorSize;
    UINT m_width;
    UINT m_height;
    float m_aspectRatio;

	// Depth buffer.
    Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;
	std::unique_ptr<DescriptorAllocator> m_rtvHeap;
	DescriptorAllocation m_rtvHeapAllocation;

	std::unique_ptr<DescriptorAllocator> m_DSVHeap;
	DescriptorAllocation m_DSVHeapAllocation;

    // Synchronization objects.
    UINT m_frameIndex;
	UINT64 frameFenceValues[FrameCount] = {};

	// Camera (root parameter 0)
	DirectX::XMFLOAT4X4 m_viewMatrix;
	DirectX::XMFLOAT4X4 m_projectionMatrix;
	DirectX::XMFLOAT4 m_cameraPosition;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_cameraCbData;

	Light m_light;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_lightCbData;

	std::unique_ptr<UploadBuffer> m_uploadBuffer;
};

DELTA_ENGINE_NS_END
