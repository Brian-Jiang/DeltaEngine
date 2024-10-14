#include "Graphics/DirectX/InstancedDrawer.h"

#include <d3dx12.h>

using namespace DeltaEngine;
using namespace DirectX;
using namespace Microsoft::WRL;

InstancedDrawer::InstancedDrawer(std::shared_ptr<Mesh> mesh, UINT32 count)
    : m_mesh(mesh), m_count(count)
{
    m_instanceData.reserve(count);
}

void InstancedDrawer::SetTransform(UINT32 index, DirectX::XMMATRIX transform) {
    XMStoreFloat4x4(&m_instanceData[index].transform, transform);
}

void InstancedDrawer::CreateBuffer(const Microsoft::WRL::ComPtr<ID3D12Device>& device) {
    UINT instanceBufferSize = sizeof(InstanceData) * m_count;
    //ComPtr<ID3D12Resource> instanceBuffer;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(instanceBufferSize);

    device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_instanceBuffer)
    );

    // Map instance data to the buffer
    InstanceData* mappedInstanceData = nullptr;
    m_instanceBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedInstanceData));
    memcpy(mappedInstanceData, m_instanceData.data(), instanceBufferSize);
    m_instanceBuffer->Unmap(0, nullptr);
}
