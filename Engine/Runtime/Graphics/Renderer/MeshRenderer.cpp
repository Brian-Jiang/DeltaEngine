#include "MeshRenderer.h"

#include <d3dx12.h>
#include <dxcapi.h>
#include <iostream>

#include "Core/DTexture.h"
#include "Graphics/DXUtils.h"
#include "IO/IOManager.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/RenderProxy/MeshRenderProxy.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace DeltaEngine;

MeshRenderer::MeshRenderer()
    : meshCount(0)
    , loadedTextureCount(0)
    , m_dirty(true)
{
}

MeshRenderer::MeshRenderer(std::string name)
    : Renderer(name)
    , meshCount(0)
    , loadedTextureCount(0)
    , m_dirty(true)
{
}

MeshRenderer::MeshRenderer(std::string name, std::shared_ptr<GameObject> gameObject)
    : Renderer(name, gameObject)
    , meshCount(0)
    , loadedTextureCount(0)
    , m_dirty(true)
{
}

MeshRenderer::~MeshRenderer()
{
}

void MeshRenderer::SetMesh(std::shared_ptr<DMesh> mesh)
{
    m_mesh = mesh;
    CreateMeshRenderProxy();
}

void MeshRenderer::InitGraphicState(std::shared_ptr<DXGraphicsContext> context)
{
    if (m_meshRenderProxy)
    {
        m_meshRenderProxy->BuildPipelineStateObject(context);
        m_meshRenderProxy->UpdateWorldTransform(GetWorldTransform());
    }

    //auto d3d12Device = context.device->GetD3D12Device();

    //// ---- Per-object constant buffer (root parameter 2) ----
    //{
    //    const UINT objectCbSize = 256;
    //    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    //    auto desc = CD3DX12_RESOURCE_DESC::Buffer(objectCbSize);
    //    ThrowIfFailed(d3d12Device->CreateCommittedResource(
    //        &heapProps,
    //        D3D12_HEAP_FLAG_NONE,
    //        &desc,
    //        D3D12_RESOURCE_STATE_GENERIC_READ,
    //        nullptr,
    //        IID_PPV_ARGS(&m_objectCb)));
    //}

    //// ---- Create vertex/index buffers and load textures for all meshes ----
    //for (size_t i = 0; i < meshes.size(); ++i)
    //{
    //    AddMesh(meshes[i], meshTransforms[i], context);
    //}
}

//void MeshRenderer::AddMesh(const Mesh* mesh, const XMMATRIX meshTransform, DXGraphicsContext& context)
//{
//    auto& uploadBuffer = EngineMain::instance->dxRenderManager->GetUploadBuffer();
//
//    // Create the vertex buffer.
//    //{
//    //    const UINT vertexBufferSize = static_cast<UINT>(mesh->vertices.size() * sizeof(Vertex));
//    //    auto addrPair = uploadBuffer.Allocate(vertexBufferSize, sizeof(Vertex));
//    //    memcpy(addrPair.CPU, mesh->vertices.data(), vertexBufferSize);
//    //    D3D12_VERTEX_BUFFER_VIEW vertexBufferView{
//    //        addrPair.GPU,
//    //        vertexBufferSize,
//    //        sizeof(Vertex)
//    //    };
//
//    //    vertexBufferViews.push_back(vertexBufferView);
//    //}
//
//    //// Create the index buffer.
//    //{
//    //    const UINT indexBufferSize = static_cast<UINT>(mesh->indices.size() * sizeof(unsigned int));
//    //    auto addrPair = uploadBuffer.Allocate(indexBufferSize, sizeof(unsigned int));
//    //    memcpy(addrPair.CPU, mesh->indices.data(), indexBufferSize);
//    //    D3D12_INDEX_BUFFER_VIEW indexBufferView{
//    //        addrPair.GPU,
//    //        indexBufferSize,
//    //        DXGI_FORMAT_R32_UINT
//    //    };
//
//    //    indexBufferViews.push_back(indexBufferView);
//    //}
//
//    // Create model matrix
//    {
//        const UINT constantBufferSize = sizeof(DirectX::XMFLOAT4X4);
//        auto addrPair = uploadBuffer.Allocate(constantBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
//        DirectX::XMStoreFloat4x4(static_cast<DirectX::XMFLOAT4X4*>(addrPair.CPU), meshTransform);
//    }
//
//    for (size_t i = 0; i < mesh->textures.size(); ++i) {
//        LoadTexture(mesh->textures[i], context);
//        ++loadedTextureCount;
//    }
//
//    ++meshCount;
//}

void DeltaEngine::MeshRenderer::CreateMeshRenderProxy()
{
    m_meshRenderProxy = std::make_shared<MeshRenderProxy>(m_mesh, std::make_shared<MeshRendererSettings>(m_settings));
}

//void MeshRenderer::LoadTexture(const std::shared_ptr<DTexture>& texture, DXGraphicsContext& context)
//{
//    if (!texture || texture->GetData().empty())
//        return;
//    context.device->CreateTextureFromFile(texture.get(), *context.commandList);
//    loadedTextures.push_back(texture);
//}

void DeltaEngine::MeshRenderer::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context)
{
    if (m_meshRenderProxy)
    {
        m_meshRenderProxy->GatherDrawCalls(context);
    }

    //auto& commandList = context.commandList;

    //DirectX::XMMATRIX rendererWorld = GetWorldTransform();

    //commandList->SetPipelineState(m_pipelineState.Get());
    //commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //struct ObjectData
    //{
    //    DirectX::XMMATRIX worldMatrix;
    //    DirectX::XMFLOAT4 color;
    //    uint32_t useInstanceMatrix;
    //} obj;
    //obj.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    //obj.useInstanceMatrix = 0;

    //for (int i = 0; i < meshCount; ++i) {
    //    // Per-draw model matrix: renderer world * mesh local transform.
    //    DirectX::XMMATRIX model = DirectX::XMMatrixMultiply(rendererWorld, meshTransforms[i]);
    //    DirectX::XMStoreFloat4x4(&obj.worldMatrix, model);

    //    void* pObj = nullptr;
    //    m_objectCb->Map(0, nullptr, &pObj);
    //    memcpy(pObj, &obj, sizeof(obj));
    //    m_objectCb->Unmap(0, nullptr);

    //    commandList->SetGraphicsRootConstantBufferView(2, m_objectCb->GetGPUVirtualAddress());
    //    commandList->IASetVertexBuffers(0, 1, &vertexBufferViews[i]);
    //    commandList->IASetIndexBuffer(&indexBufferViews[i]);

    //    if (i < static_cast<int>(loadedTextures.size()))
    //        commandList->SetShaderResourceView(1, loadedTextures[i]);

    //    commandList->DrawIndexed(static_cast<uint32_t>(meshes[i]->indices.size()), 1, 0, 0, 0);
    //}
}

void DeltaEngine::MeshRenderer::OnTransformChanged()
{
    if (m_meshRenderProxy)
    {
        m_meshRenderProxy->UpdateWorldTransform(GetWorldTransform());
    }
}
