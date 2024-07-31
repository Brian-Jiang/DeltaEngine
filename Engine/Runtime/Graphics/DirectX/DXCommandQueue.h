#pragma once

#include "EngineIncludes.h"

#include <chrono>
#include <queue>
#include <d3d12.h>
#include <wrl.h>

DELTA_ENGINE_NS_BEGIN

class DXCommandQueue
{
public:
    DXCommandQueue(Microsoft::WRL::ComPtr<ID3D12Device4> device, D3D12_COMMAND_LIST_TYPE type);
    ~DXCommandQueue();

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> CreateCommandList(
        const Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& allocator, 
        const Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState = nullptr);

    bool IsFenceComplete(UINT64 fenceValue);
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> GetCommandList(const Microsoft::WRL::ComPtr<ID3D12PipelineState>& pipelineState = nullptr);
    UINT64 ExecuteCommandList(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7>& commandList);
    void WaitForFenceValue(UINT64 fenceValue, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
    void WaitForLastAllocator();

    UINT64 Signal();
    void Flush();

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue() { return commandQueue; }

private:
    struct AllocatorStruct {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
        UINT64 fenceValue;
    };

    std::queue<AllocatorStruct> allocatorQueue;
    std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7>> commandListQueue;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    UINT64 fenceValue;
    HANDLE fenceEvent;

    Microsoft::WRL::ComPtr<ID3D12Device4> device;
    D3D12_COMMAND_LIST_TYPE type;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
};

DELTA_ENGINE_NS_END
