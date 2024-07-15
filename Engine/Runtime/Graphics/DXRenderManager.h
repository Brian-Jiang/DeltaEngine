#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>

namespace DeltaEngine
{

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
    void OnDestroy();

	Microsoft::WRL::ComPtr<ID3D12Device4> GetDevice() { return m_device; }
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return m_commandList; }
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetSRVHeap() { return m_srvHeap; }

private:
    // void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter);
    // std::wstring GetAssetFullPath(LPCWSTR assetName);

	HWND hwnd;

    // By default, enable V-Sync.
	// Can be toggled with the V key.
	bool g_VSync = true;
	bool g_TearingSupported = false;
	// By default, use windowed mode.
	// Can be toggled with the Alt+Enter or F11
	bool g_Fullscreen = false;

    static const UINT FrameCount = 2;

    bool m_useWarpDevice;
	D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device4> m_device;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    UINT m_rtvDescriptorSize;
    UINT m_width;
    UINT m_height;
    float m_aspectRatio;

    // App resources.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;
};

}
