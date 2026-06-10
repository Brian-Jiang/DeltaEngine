#include "Shared/GpuD3D12Validation.h"

#include "Runtime/Graphics/DirectX/Device.h"

#include <dxgi1_6.h>
#include <gtest/gtest.h>
#include <wrl.h>

namespace DeltaEngine::Tests
{

bool IsD3D12DebugLayerAvailable()
{
    Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
    return SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
}

bool IsWarpAdapterAvailable()
{
    Microsoft::WRL::ComPtr<IDXGIFactory6> dxgiFactory6;
    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;

    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    if (FAILED(::CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory6)))
        return false;

    return SUCCEEDED(dxgiFactory6->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter)));
}

void EnableDebugLayerForTests()
{
    Device::EnableDebugLayer();
}

void DisableBreakOnSeverity(ID3D12Device* device)
{
    if (!device)
        return;

    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        return;

    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, FALSE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, FALSE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, FALSE);
}

std::vector<GpuValidationMessage> DrainD3D12InfoQueue(ID3D12Device* device)
{
    std::vector<GpuValidationMessage> messages;
    if (!device)
        return messages;

    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        return messages;

    const UINT64 messageCount = infoQueue->GetNumStoredMessages();
    messages.reserve(static_cast<size_t>(messageCount));

    for (UINT64 i = 0; i < messageCount; ++i)
    {
        SIZE_T messageLength = 0;
        infoQueue->GetMessage(i, nullptr, &messageLength);

        std::vector<BYTE> buffer(messageLength);
        auto* message = reinterpret_cast<D3D12_MESSAGE*>(buffer.data());
        if (FAILED(infoQueue->GetMessage(i, message, &messageLength)))
            continue;

        GpuValidationMessage entry;
        entry.severity = message->Severity;
        entry.id = message->ID;
        if (message->pDescription)
            entry.text = message->pDescription;
        messages.push_back(std::move(entry));
    }

    infoQueue->ClearStoredMessages();
    return messages;
}

void AssertGpuValidationClean(ID3D12Device* device)
{
    const std::vector<GpuValidationMessage> messages = DrainD3D12InfoQueue(device);
    for (const GpuValidationMessage& message : messages)
    {
        if (message.severity == D3D12_MESSAGE_SEVERITY_CORRUPTION
            || message.severity == D3D12_MESSAGE_SEVERITY_ERROR)
        {
            ADD_FAILURE() << "D3D12 validation " << message.text;
        }
    }
}

} // namespace DeltaEngine::Tests
