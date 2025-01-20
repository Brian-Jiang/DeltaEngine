#include "Runtime/Graphics/DirectX/CommandList.h"

#include <d3dx12.h>
#include <assert.h>
#include "Runtime/Graphics/DirectX/Resource.h"
#include "Runtime/Graphics/DirectX/ResourceStateTracker.h"
#include "Runtime/Graphics/DirectX/UploadBuffer.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DynamicDescriptorHeap.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/DirectX/Texture.h"
#include "Runtime/Graphics/DirectX/Buffer.h"
#include "Runtime/Graphics/DirectX/ConstantBuffer.h"
#include "Runtime/Graphics/DirectX/ShaderResourceView.h"
#include "Runtime/Graphics/DirectX/UnorderedAccessView.h"
#include "Runtime/Graphics/DirectX/ConstantBufferView.h"
#include "Runtime/Graphics/DXUtils.h"

using namespace DeltaEngine;

DeltaEngine::CommandList::CommandList(Device& device, D3D12_COMMAND_LIST_TYPE type)
    : m_Device(device), m_d3d12CommandListType(type), m_RootSignature(nullptr), m_PipelineState(nullptr)
{
    auto d3d12Device = m_Device.GetD3D12Device();

    ThrowIfFailed(
        d3d12Device->CreateCommandAllocator(m_d3d12CommandListType, IID_PPV_ARGS(&m_d3d12CommandAllocator)));

    ThrowIfFailed(d3d12Device->CreateCommandList(0, m_d3d12CommandListType, m_d3d12CommandAllocator.Get(), nullptr,
        IID_PPV_ARGS(&m_d3d12CommandList)));

    m_UploadBuffer = std::make_unique<UploadBuffer>(device, 2 * 1024 * 1024);

    m_ResourceStateTracker = std::make_unique<ResourceStateTracker>();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
        m_DynamicDescriptorHeap[i] =
            std::make_unique<DynamicDescriptorHeap>(device, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
        m_DescriptorHeaps[i] = nullptr;
    }
}

void DeltaEngine::CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap)
{
    if (m_DescriptorHeaps[heapType] != heap) {
        m_DescriptorHeaps[heapType] = heap;

        UINT numDescriptorHeaps = 0;
        ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

        for (UINT32 i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
            ID3D12DescriptorHeap* descriptorHeap = m_DescriptorHeaps[i];
            if (descriptorHeap) {
                descriptorHeaps[numDescriptorHeaps++] = descriptorHeap;
            }
        }

        m_d3d12CommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
    }
}

void CommandList::TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter,
    UINT subresource, bool flushBarriers) {
    if (resource) {
        // The "before" state is not important. It will be resolved by the resource state tracker.
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter,
            subresource);
        m_ResourceStateTracker->ResourceBarrier(barrier);
    }

    if (flushBarriers) {
        FlushResourceBarriers();
    }
}

void CommandList::TransitionBarrier(const std::shared_ptr<Resource>& resource, D3D12_RESOURCE_STATES stateAfter,
    UINT subresource, bool flushBarriers) {
    if (resource) {
        TransitionBarrier(resource->GetD3D12Resource(), stateAfter, subresource, flushBarriers);
    }
}

void CommandList::CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstRes,
    Microsoft::WRL::ComPtr<ID3D12Resource> srcRes) {
    assert(dstRes);
    assert(srcRes);

    TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

    FlushResourceBarriers();

    m_d3d12CommandList->CopyResource(dstRes.Get(), srcRes.Get());

    TrackResource(dstRes);
    TrackResource(srcRes);
}

void CommandList::CopyResource(const std::shared_ptr<Resource>& dstRes, const std::shared_ptr<Resource>& srcRes) {
    assert(dstRes && srcRes);

    CopyResource(dstRes->GetD3D12Resource(), srcRes->GetD3D12Resource());
}

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes,
    const void* bufferData) {
    // Constant buffers must be 256-byte aligned.
    auto heapAllococation = m_UploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    memcpy(heapAllococation.CPU, bufferData, sizeInBytes);

    m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.GPU);
}

void CommandList::SetGraphicsRootSignature(const std::shared_ptr<RootSignature>& rootSignature) {
    assert(rootSignature);

    auto d3d12RootSignature = rootSignature->GetD3D12RootSignature().Get();
    if (m_RootSignature != d3d12RootSignature) {
        m_RootSignature = d3d12RootSignature;

        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
            m_DynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
        }

        m_d3d12CommandList->SetGraphicsRootSignature(m_RootSignature);

        TrackResource(m_RootSignature);
    }
}

void CommandList::SetConstantBufferView(uint32_t rootParameterIndex, const std::shared_ptr<ConstantBuffer>& buffer,
    D3D12_RESOURCE_STATES stateAfter, size_t bufferOffset) {
    if (buffer) {
        auto d3d12Resource = buffer->GetD3D12Resource();
        TransitionBarrier(d3d12Resource, stateAfter);

        m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageInlineCBV(
            rootParameterIndex, d3d12Resource->GetGPUVirtualAddress() + bufferOffset);

        TrackResource(buffer);
    }
}

void CommandList::SetShaderResourceView(uint32_t rootParameterIndex, const std::shared_ptr<Buffer>& buffer,
    D3D12_RESOURCE_STATES stateAfter, size_t bufferOffset) {
    if (buffer) {
        auto d3d12Resource = buffer->GetD3D12Resource();
        TransitionBarrier(d3d12Resource, stateAfter);

        m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageInlineSRV(
            rootParameterIndex, d3d12Resource->GetGPUVirtualAddress() + bufferOffset);

        TrackResource(buffer);
    }
}

void CommandList::SetUnorderedAccessView(uint32_t rootParameterIndex, const std::shared_ptr<Buffer>& buffer,
    D3D12_RESOURCE_STATES stateAfter, size_t bufferOffset) {
    if (buffer) {
        auto d3d12Resource = buffer->GetD3D12Resource();
        TransitionBarrier(d3d12Resource, stateAfter);

        m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageInlineUAV(
            rootParameterIndex, d3d12Resource->GetGPUVirtualAddress() + bufferOffset);

        TrackResource(buffer);
    }
}

void CommandList::SetShaderResourceView(uint32_t rootParameterIndex, uint32_t descriptorOffset,
    const std::shared_ptr<ShaderResourceView>& srv,
    D3D12_RESOURCE_STATES stateAfter, UINT firstSubresource, UINT numSubresources) {
    assert(srv);

    auto resource = srv->GetResource();
    if (resource) {
        if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
            for (uint32_t i = 0; i < numSubresources; ++i) {
                TransitionBarrier(resource, stateAfter, firstSubresource + i);
            }
        }
        else {
            TransitionBarrier(resource, stateAfter);
        }

        TrackResource(resource);
    }

    m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(
        rootParameterIndex, descriptorOffset, 1, srv->GetDescriptorHandle());
}

void CommandList::SetShaderResourceView(int32_t rootParameterIndex, uint32_t descriptorOffset,
    const std::shared_ptr<Texture>& texture, D3D12_RESOURCE_STATES stateAfter,
    UINT firstSubresource, UINT numSubresources) {
    if (texture) {
        if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
            for (uint32_t i = 0; i < numSubresources; ++i) {
                TransitionBarrier(texture, stateAfter, firstSubresource + i);
            }
        }
        else {
            TransitionBarrier(texture, stateAfter);
        }

        TrackResource(texture);

        m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(
            rootParameterIndex, descriptorOffset, 1, texture->GetShaderResourceView());
    }
}

void CommandList::SetUnorderedAccessView(uint32_t rootParameterIndex, uint32_t descriptorOffset,
    const std::shared_ptr<UnorderedAccessView>& uav,
    D3D12_RESOURCE_STATES stateAfter, UINT firstSubresource,
    UINT numSubresources) {
    assert(uav);

    auto resource = uav->GetResource();
    if (resource) {
        if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
            for (uint32_t i = 0; i < numSubresources; ++i) {
                TransitionBarrier(resource, stateAfter, firstSubresource + i);
            }
        }
        else {
            TransitionBarrier(resource, stateAfter);
        }

        TrackResource(resource);
    }

    m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(
        rootParameterIndex, descriptorOffset, 1, uav->GetDescriptorHandle());
}

void CommandList::SetUnorderedAccessView(uint32_t rootParameterIndex, uint32_t descriptorOffset,
    const std::shared_ptr<Texture>& texture, UINT mip,
    D3D12_RESOURCE_STATES stateAfter, UINT firstSubresource,
    UINT numSubresources) {
    if (texture) {
        if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
            for (uint32_t i = 0; i < numSubresources; ++i) {
                TransitionBarrier(texture, stateAfter, firstSubresource + i);
            }
        }
        else {
            TransitionBarrier(texture, stateAfter);
        }

        TrackResource(texture);

        m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(
            rootParameterIndex, descriptorOffset, 1, texture->GetUnorderedAccessView(mip));
    }
}

void CommandList::SetConstantBufferView(uint32_t rootParameterIndex, uint32_t descriptorOffset,
    const std::shared_ptr<ConstantBufferView>& cbv,
    D3D12_RESOURCE_STATES                      stateAfter) {
    assert(cbv);

    auto constantBuffer = cbv->GetConstantBuffer();
    if (constantBuffer) {
        TransitionBarrier(constantBuffer, stateAfter);
        TrackResource(constantBuffer);
    }

    m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(
        rootParameterIndex, descriptorOffset, 1, cbv->GetDescriptorHandle());
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance) {
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
        m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
    }

    m_d3d12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex,
    uint32_t startInstance) {
    FlushResourceBarriers();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
        m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
    }

    m_d3d12CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void CommandList::ResolveSubresource(const std::shared_ptr<Resource>& dstRes, const std::shared_ptr<Resource>& srcRes,
    uint32_t dstSubresource, uint32_t srcSubresource) {
    assert(dstRes && srcRes);

    TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_RESOLVE_DEST, dstSubresource);
    TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, srcSubresource);

    FlushResourceBarriers();

    m_d3d12CommandList->ResolveSubresource(dstRes->GetD3D12Resource().Get(), dstSubresource,
        srcRes->GetD3D12Resource().Get(), srcSubresource,
        dstRes->GetD3D12ResourceDesc().Format);

    TrackResource(srcRes);
    TrackResource(dstRes);
}

void CommandList::TrackResource(Microsoft::WRL::ComPtr<ID3D12Object> object) {
    m_TrackedObjects.push_back(object);
}

void CommandList::TrackResource(const std::shared_ptr<Resource>& res) {
    assert(res);

    TrackResource(res->GetD3D12Resource());
}

void CommandList::ReleaseTrackedObjects() {
    m_TrackedObjects.clear();
}


void CommandList::FlushResourceBarriers() {
    m_ResourceStateTracker->FlushResourceBarriers(shared_from_this());
}

void CommandList::Reset() {
    ThrowIfFailed(m_d3d12CommandAllocator->Reset());
    ThrowIfFailed(m_d3d12CommandList->Reset(m_d3d12CommandAllocator.Get(), nullptr));

    m_ResourceStateTracker->Reset();
    m_UploadBuffer->Reset();

    ReleaseTrackedObjects();

    for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
        m_DynamicDescriptorHeap[i]->Reset();
        m_DescriptorHeaps[i] = nullptr;
    }

    m_RootSignature = nullptr;
    m_PipelineState = nullptr;
    //m_ComputeCommandList = nullptr;
}

bool CommandList::Close(const std::shared_ptr<CommandList>& pendingCommandList) {
    // Flush any remaining barriers.
    FlushResourceBarriers();

    m_d3d12CommandList->Close();

    // Flush pending resource barriers.
    uint32_t numPendingBarriers = m_ResourceStateTracker->FlushPendingResourceBarriers(pendingCommandList);
    // Commit the final resource state to the global state.
    m_ResourceStateTracker->CommitFinalResourceStates();

    return numPendingBarriers > 0;
}

void CommandList::Close() {
    FlushResourceBarriers();
    m_d3d12CommandList->Close();
}