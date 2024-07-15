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

class DXUtils
{

public:

	static Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);

    static Microsoft::WRL::ComPtr<ID3D12Device4> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);

	static Microsoft::WRL::ComPtr<ID3D12CommandQueue> CreateCommandQueue(
		Microsoft::WRL::ComPtr<ID3D12Device4> device, D3D12_COMMAND_LIST_TYPE type);

	static bool CheckTearingSupport();

	static Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain(
		HWND hWnd, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount);

	static Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
		Microsoft::WRL::ComPtr<ID3D12Device4> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

	static Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(
		Microsoft::WRL::ComPtr<ID3D12Device4> device, D3D12_COMMAND_LIST_TYPE type);

	static Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CreateCommandList(
		Microsoft::WRL::ComPtr<ID3D12Device4> device, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);

};

}
