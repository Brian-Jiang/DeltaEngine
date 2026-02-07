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
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/Device.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class RootSignature;
class SwapChain;

class DXRenderManager
{
public:
	DXRenderManager(HWND hwnd, UINT width, UINT height);
	void LoadPipeline();
    void LoadAssets();
    void InitFinish();
    void PrepareFrame();
    void RenderFrame();
    void WaitForPreviousFrame();
	void Resize(UINT width, UINT height);
	void SetFullscreen(bool fullscreen);
	void ResizeDepthBuffer(int width, int height);
    void OnDestroy();

	DXGraphicsContext GetGraphicsContext() const;

	CommandQueue& GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;
    UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

	UploadBuffer& GetUploadBuffer() { return *m_uploadBuffer; }

	void ToggleVSync(bool enableVSync) { g_VSync = enableVSync; }

	Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice() { return m_device->GetD3D12Device(); }
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return m_commandList; }
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSRVHeap() { return m_srvHeap; }
	bool IsFullscreen() { return g_Fullscreen; }
	bool IsVSync() { return g_VSync; }

	UINT GetWidth() { return m_width; }
	UINT GetHeight() { return m_height; }

	void SetMVPMatrix(DirectX::XMMATRIX mvp) { mvpMatrix = mvp; }
	void SetCameraPosition(DirectX::XMVECTOR cam) { DirectX::XMStoreFloat4(&cameraPosition, cam); }

	//void SetModelMatrix(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, DirectX::XMMATRIX model);
	//void ResetModelMatrix(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

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

	//DXCommandQueue *directCommandQueue;
	//DXCommandQueue *copyCommandQueue;

    bool m_useWarpDevice;

	CD3DX12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;

	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	//Microsoft::WRL::ComPtr<ID3D12Device4> m_device;
	std::shared_ptr<Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	// Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> m_commandList;

    UINT m_rtvDescriptorSize;
    UINT m_width;
    UINT m_height;
    float m_aspectRatio;

	// Depth buffer.
    Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthBuffer;
    // Descriptor heap for depth buffer.
    //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
	std::unique_ptr<DescriptorAllocator> m_rtvHeap;
	DescriptorAllocation m_rtvHeapAllocation;

	std::unique_ptr<DescriptorAllocator> m_DSVHeap;
	DescriptorAllocation m_DSVHeapAllocation;

    // Synchronization objects.
    UINT m_frameIndex;
	UINT64 frameFenceValues[FrameCount] = {};

	DirectX::XMMATRIX mvpMatrix;
	DirectX::XMFLOAT4 cameraPosition;
	//DirectX::XMMATRIX m_ModelMatrix;
	Light m_light;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_lightCbData;

	std::unique_ptr<UploadBuffer> m_uploadBuffer;
};

DELTA_ENGINE_NS_END
