#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT

#include <dxgi1_6.h>
// #include <d3d12.h>
#include <d3dx12.h>

namespace DeltaEngine
{

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
static inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

#define _KB(x) (x * 1024)
#define _MB(x) (x * 1024 * 1024)

#define _64KB _KB(64)
#define _1MB _MB(1)
#define _2MB _MB(2)
#define _4MB _MB(4)
#define _8MB _MB(8)
#define _16MB _MB(16)
#define _32MB _MB(32)
#define _64MB _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)

class DXUtils
{

public:

	static Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);

    static Microsoft::WRL::ComPtr<ID3D12Device4> CreateDevice(const Microsoft::WRL::ComPtr<IDXGIAdapter4>& adapter);

	static Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(
        const Microsoft::WRL::ComPtr<ID3D12Device4>& device, D3D12_COMMAND_LIST_TYPE type);

	static bool CheckTearingSupport();

	static Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(
		HWND hWnd, const Microsoft::WRL::ComPtr<ID3D12CommandQueue>& commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount);

	static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
        const Microsoft::WRL::ComPtr<ID3D12Device4>& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

	static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(
        const Microsoft::WRL::ComPtr<ID3D12Device4>& device, D3D12_COMMAND_LIST_TYPE type);

	static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(
        const Microsoft::WRL::ComPtr<ID3D12Device4>& device, const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& commandAllocator, D3D12_COMMAND_LIST_TYPE type);

	static Microsoft::WRL::ComPtr<ID3D12Fence> CreateFence(const Microsoft::WRL::ComPtr<ID3D12Device4>& device, UINT64 fenceValue);

	static void UpdateBufferResource(
        const Microsoft::WRL::ComPtr<ID3D12Device>& device,
        const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>& commandList,
        ID3D12Resource** pDestinationResource, 
        ID3D12Resource** pIntermediateResource,
        size_t numElements, size_t elementSize, const void* bufferData, 
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	static void ReportLiveDXGIObjects();
};

}
