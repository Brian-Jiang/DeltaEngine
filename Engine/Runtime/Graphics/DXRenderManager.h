#pragma once

#include "EngineIncludes.h"

#include <chrono>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMath.h>

#include "DirectX/DXCommandQueue.h"

DELTA_ENGINE_NS_BEGIN

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

	DXCommandQueue *GetCommandQueue(D3D12_COMMAND_LIST_TYPE type) const;

	void ToggleVSync(bool enableVSync) { g_VSync = enableVSync; }

	Microsoft::WRL::ComPtr<ID3D12Device4> GetDevice() { return m_device; }
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return m_commandList; }
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSRVHeap() { return m_srvHeap; }
	bool IsFullscreen() { return g_Fullscreen; }
	bool IsVSync() { return g_VSync; }

	UINT GetWidth() { return m_width; }
	UINT GetHeight() { return m_height; }

	void SetMVPMatrix(DirectX::XMMATRIX mvp) { mvpMatrix = mvp; }
	void SetModelMatrix(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, DirectX::XMMATRIX model);
	void ResetModelMatrix(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);

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

	DXCommandQueue *directCommandQueue;
	DXCommandQueue *copyCommandQueue;

    bool m_useWarpDevice;

	CD3DX12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;

	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device4> m_device;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	// Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
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
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

    // Synchronization objects.
    UINT m_frameIndex;
	UINT64 frameFenceValues[FrameCount] = {};

	DirectX::XMMATRIX mvpMatrix;
	//DirectX::XMMATRIX m_ModelMatrix;
};

DELTA_ENGINE_NS_END
