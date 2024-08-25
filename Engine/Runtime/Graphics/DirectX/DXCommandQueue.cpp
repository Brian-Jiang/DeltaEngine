#include "Graphics/DirectX/DXCommandQueue.h"

#include "Graphics/DXUtils.h"

using namespace DeltaEngine;
using namespace Microsoft::WRL;

DXCommandQueue::DXCommandQueue(ComPtr<ID3D12Device4> device, D3D12_COMMAND_LIST_TYPE type): device(device), type(type), fenceValue(0)
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = type;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
    fence = DXUtils::CreateFence(device, fenceValue);
    fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

DXCommandQueue::~DXCommandQueue()
{
    CloseHandle(fenceEvent);
}

ComPtr<ID3D12CommandAllocator> DXCommandQueue::CreateCommandAllocator()
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList5> DXCommandQueue::CreateCommandList(const ComPtr<ID3D12CommandAllocator>& allocator, const ComPtr<ID3D12PipelineState>& pipelineState)
{
    ComPtr<ID3D12GraphicsCommandList5> commandList;
    ThrowIfFailed(device->CreateCommandList(0, type, allocator.Get(), pipelineState.Get(), IID_PPV_ARGS(&commandList)));

    return commandList;
}

bool DXCommandQueue::IsFenceComplete(UINT64 fenceValue)
{
    // if (fenceValue == 0)
    // {
    //     return true;
    // }

    return fence->GetCompletedValue() >= fenceValue;
}

ComPtr<ID3D12GraphicsCommandList5> DXCommandQueue::GetCommandList(const ComPtr<ID3D12PipelineState>& pipelineState)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12GraphicsCommandList5> commandList;

    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    if (!commandListQueue.empty() && IsFenceComplete(allocatorQueue.front().fenceValue))
    {
        commandAllocator = allocatorQueue.front().allocator;
        allocatorQueue.pop();

        ThrowIfFailed(commandAllocator->Reset());
    }
    else
    {
        commandAllocator = CreateCommandAllocator();
    }

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    if (!commandListQueue.empty())
    {
        commandList = commandListQueue.front();
        commandListQueue.pop();
 
        ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));
    }
    else
    {
        commandList = CreateCommandList(commandAllocator);
    }

    // Associate the command allocator with the command list so that it can be
    // retrieved when the command list is executed.
    ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));
 
    return commandList;
}

UINT64 DXCommandQueue::ExecuteCommandList(const ComPtr<ID3D12GraphicsCommandList5>& commandList)
{
    // Command list needs to be closed before it can be executed. This is when the validation of the command list happens.
    ThrowIfFailed(commandList->Close());
 
    ID3D12CommandAllocator* commandAllocator;
    UINT dataSize = sizeof(commandAllocator);
    ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));
 
    ID3D12CommandList* const ppCommandLists[] = {
        commandList.Get()
    };
 
    commandQueue->ExecuteCommandLists(1, ppCommandLists);

    auto newFenceValue = Signal();

    // Command allocator is now managed by COM pointer and associate with the fence value.
    allocatorQueue.emplace(AllocatorStruct{ commandAllocator, newFenceValue });

    // Command list can be reused after it has been executed.
    commandListQueue.push(commandList);

    // The ownership of the command allocator has been transferred to the ComPtr
    // in the command allocator queue. It is safe to release the reference 
    // in this temporary COM pointer here.
    commandAllocator->Release();
 
    return newFenceValue;
}

void DXCommandQueue::WaitForFenceValue(UINT64 fenceValue, std::chrono::milliseconds duration)
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
    }
}

void DXCommandQueue::WaitForLastAllocator()
{
    if (!allocatorQueue.empty())
    {
        WaitForFenceValue(allocatorQueue.back().fenceValue);
    }
}

UINT64 DXCommandQueue::Signal()
{
    // Get the highest fence value.
    const UINT64 newFenceValue = fenceValue++;

    // Add a signal command to the queue so that when the command queue finishes executing, it will signal the fence.
    // Then the m_fence will be signaled with the value of the fence.
    ThrowIfFailed(commandQueue->Signal(fence.Get(), newFenceValue));

    return newFenceValue;
}

void DXCommandQueue::Flush()
{
    WaitForFenceValue(Signal());
}
