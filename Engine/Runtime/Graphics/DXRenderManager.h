#pragma once
#include <dxgi1_6.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <dxcapi.h>

using namespace DirectX;
using namespace Microsoft::WRL;

class DXRenderManager
{
public:
	DXRenderManager(HWND hwnd, UINT width, UINT height);
	void LoadPipeline();
    void LoadAssets();
    void InitFinish();
    void PrepareFrame();
    void RenderFrame();
    // void OnRender();
    // void PopulateCommandList();
    void WaitForPreviousFrame();

    ComPtr<ID3D12Device> GetDevice() { return m_device; }
    ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return m_commandList; }
    ComPtr<ID3D12DescriptorHeap> GetSRVHeap() { return m_srvHeap; }

private:
    void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter);
    // std::wstring GetAssetFullPath(LPCWSTR assetName);

	HWND hwnd;

    static const UINT FrameCount = 2;

    bool m_useWarpDevice;
	D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    UINT m_rtvDescriptorSize;
    UINT m_width;
    UINT m_height;
    float m_aspectRatio;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };
};

