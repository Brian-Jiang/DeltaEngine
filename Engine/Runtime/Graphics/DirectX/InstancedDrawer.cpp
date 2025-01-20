#include "Graphics/DirectX/InstancedDrawer.h"

#include <d3dx12.h>

#include "EngineMain.h"

using namespace DeltaEngine;
using namespace DirectX;
using namespace Microsoft::WRL;

InstancedDrawer::InstancedDrawer(std::shared_ptr<Mesh> mesh, UINT32 count)
    : m_mesh(mesh), m_count(count)
{
    m_instanceData.resize(count);
}

void InstancedDrawer::SetTransform(UINT32 index, DirectX::XMMATRIX transform) {
    XMStoreFloat4x4(&m_instanceData[index].transform, transform);
}

void InstancedDrawer::SetColor(UINT32 index, DirectX::XMFLOAT3 color) {
    memcpy(&m_instanceData[index].color, &color, sizeof(XMFLOAT3));
}

void InstancedDrawer::CreateBuffer(const Microsoft::WRL::ComPtr<ID3D12Device>& device) {
    auto& uploadBuffer = EngineMain::instance->dxRenderManager->GetUploadBuffer();

    // Create the vertex buffer.
    {
        const UINT vertexBufferSize = static_cast<UINT>(m_mesh->vertices.size() * sizeof(Vertex));
        auto addrPair = uploadBuffer.Allocate(vertexBufferSize, sizeof(Vertex));
        memcpy(addrPair.CPU, m_mesh->vertices.data(), vertexBufferSize);
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView{
            addrPair.GPU,
            vertexBufferSize,
            sizeof(Vertex)
        };

        m_vertexBufferView = vertexBufferView;
        //vertexBufferViews.push_back(vertexBufferView);
    }

    // Create the index buffer.
    {
        const UINT indexBufferSize = static_cast<UINT>(m_mesh->indices.size() * sizeof(unsigned int));
        auto addrPair = uploadBuffer.Allocate(indexBufferSize, sizeof(unsigned int));
        memcpy(addrPair.CPU, m_mesh->indices.data(), indexBufferSize);
        D3D12_INDEX_BUFFER_VIEW indexBufferView{
            addrPair.GPU,
            indexBufferSize,
            DXGI_FORMAT_R32_UINT
        };

        m_indexBufferView = indexBufferView;
        //indexBufferViews.push_back(indexBufferView);
    }


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

    m_instanceBufferView.BufferLocation = m_instanceBuffer->GetGPUVirtualAddress();
    m_instanceBufferView.SizeInBytes = instanceBufferSize;
    m_instanceBufferView.StrideInBytes = sizeof(InstanceData);
}

void InstancedDrawer::Draw(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) {
    commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetVertexBuffers(1, 1, &m_instanceBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);
    commandList->DrawIndexedInstanced(m_mesh->indices.size(), m_count, 0, 0, 0);
}
