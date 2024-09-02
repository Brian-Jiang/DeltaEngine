#include "MeshRenderer.h"

#include <d3dx12.h>
#include "Graphics/Texture.h"
#include "PlatformHelpers.h"
#include "Runtime/Graphics/Mesh.h"
#include "Runtime/EngineMain.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace DeltaEngine;

MeshRenderer::MeshRenderer() : meshCount(0)
{
}

MeshRenderer::~MeshRenderer()
{
}

void MeshRenderer::Start(const std::vector<Mesh*> meshes, const std::vector<DirectX::XMMATRIX> meshTransforms, const ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12DescriptorHeap> srvHeap)
{
	this->meshes = meshes;
	this->meshTransforms = meshTransforms;
    for (size_t i = 0; i < meshes.size(); ++i) {
		auto mesh = meshes[i];
		auto meshTransform = meshTransforms[i];
		AddMesh(mesh, meshTransform, device, commandList, srvHeap);
	}

    // 
    // Create the vertex buffer.
    //{
    //    const UINT vertexBufferSize = static_cast<UINT>(mesh.vertices.size() * sizeof(Vertex));
    //    //const UINT vertexBufferSize = sizeof(mesh.vertices);

    //    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    //    auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
    //    ThrowIfFailed(device->CreateCommittedResource(
    //        &heapProps,
    //        D3D12_HEAP_FLAG_NONE,
    //        &desc,
    //        D3D12_RESOURCE_STATE_GENERIC_READ,
    //        nullptr,
    //        IID_PPV_ARGS(&m_vertexBuffer)));

    //    UINT8* pVertexDataBegin;
    //    CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
    //    ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
    //    memcpy(pVertexDataBegin, mesh.vertices.data(), vertexBufferSize);
    //    m_vertexBuffer->Unmap(0, nullptr);

    //    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    //    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    //    m_vertexBufferView.SizeInBytes = vertexBufferSize;
    //}

    //// Create the index buffer.
    //{
    //    const UINT indexBufferSize = static_cast<UINT>(mesh.indices.size() * sizeof(unsigned int));
    //    //const UINT indexBufferSize = sizeof(mesh.indices);

    //    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    //    auto desc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
    //    ThrowIfFailed(device->CreateCommittedResource(
    //        &heapProps,
    //        D3D12_HEAP_FLAG_NONE,
    //        &desc,
    //        D3D12_RESOURCE_STATE_GENERIC_READ,
    //        nullptr,
    //        IID_PPV_ARGS(&m_indexBuffer)));

    //    UINT8* pIndexDataBegin;
    //    CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
    //    ThrowIfFailed(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
    //    memcpy(pIndexDataBegin, mesh.indices.data(), indexBufferSize);
    //    m_indexBuffer->Unmap(0, nullptr);

    //    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    //    m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    //    //m_indexBufferView.StrideInBytes = sizeof(uint32_t);
    //    m_indexBufferView.SizeInBytes = indexBufferSize;
    //}

    // Create the textures (diffuse and normal).
	//for (auto texture : mesh.textures) {
	//	LoadTexture(texture, device, commandList, srvHeap, 0);
	//}

    //LoadTexture(diffuseTexturePath, device, commandList, srvHeap, 0);
    //LoadTexture(normalTexturePath, device, commandList, srvHeap, 1);
}

void MeshRenderer::LoadTexture(const Texture* texture, const ComPtr<ID3D12Device>& device, ComPtr<ID3D12GraphicsCommandList>& commandList, ComPtr<ID3D12DescriptorHeap>& srvHeap, UINT descriptorIndex)
{
    //auto texture = Texture::LoadFromFile(texturePath);
    auto textureHeight = texture->GetHeight();
    auto textureWidth = texture->GetWidth();

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = textureWidth;
    textureDesc.Height = textureHeight;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ComPtr<ID3D12Resource> textureResource;
    ThrowIfFailed(device->CreateCommittedResource(
        &hp,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&textureResource)));

    UINT64 rowPitch = textureWidth * TexturePixelSize;
    UINT64 alignedRowPitch = (rowPitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    UINT64 textureSize = alignedRowPitch * textureHeight;

    hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(textureSize);
    ComPtr<ID3D12Resource> textureUploadHeap;
    ThrowIfFailed(device->CreateCommittedResource(
        &hp,
        D3D12_HEAP_FLAG_NONE,
        &uploadHeapDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&textureUploadHeap)));

    auto rawData = texture->GetData().data();
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = rawData;
    textureData.RowPitch = alignedRowPitch;
    textureData.SlicePitch = alignedRowPitch * textureHeight;

    D3D12_SUBRESOURCE_FOOTPRINT pitchedDesc = {};
    pitchedDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    pitchedDesc.Width = textureWidth;
    pitchedDesc.Height = textureHeight;
    pitchedDesc.Depth = 1;
    pitchedDesc.RowPitch = alignedRowPitch;

    UINT8* pData;
    textureUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    for (int y = 0; y < textureHeight; y++) {
        memcpy(pData + y * alignedRowPitch, rawData + y * rowPitch, rowPitch);
    }
    textureUploadHeap->Unmap(0, nullptr);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTexture2D = { 0 };
    placedTexture2D.Offset = 0;
    placedTexture2D.Footprint = pitchedDesc;

    D3D12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(textureResource.Get(), 0);
    D3D12_TEXTURE_COPY_LOCATION src = CD3DX12_TEXTURE_COPY_LOCATION(textureUploadHeap.Get(), placedTexture2D);
    commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    auto rb = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &rb);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(srvHeap->GetCPUDescriptorHandleForHeapStart(), descriptorIndex, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    device->CreateShaderResourceView(textureResource.Get(), &srvDesc, srvHandle);
}

void MeshRenderer::AddMesh(const Mesh* mesh, const XMMATRIX meshTransform, const Microsoft::WRL::ComPtr<ID3D12Device>& device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& srvHeap) {
    // Create the vertex buffer.
    {
        const UINT vertexBufferSize = static_cast<UINT>(mesh->vertices.size() * sizeof(Vertex));

        ComPtr<ID3D12Resource> vertexBuffer;
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&vertexBuffer)));

        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, mesh->vertices.data(), vertexBufferSize);
        vertexBuffer->Unmap(0, nullptr);
		vertexBuffers.push_back(vertexBuffer);

        D3D12_VERTEX_BUFFER_VIEW vertexBufferView {
			vertexBuffer->GetGPUVirtualAddress(),
			vertexBufferSize,
			sizeof(Vertex)
        };
		vertexBufferViews.push_back(vertexBufferView);
    }

    // Create the index buffer.
    {
        const UINT indexBufferSize = static_cast<UINT>(mesh->indices.size() * sizeof(unsigned int));
        //const UINT indexBufferSize = sizeof(mesh.indices);

		ComPtr<ID3D12Resource> indexBuffer;
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
        auto desc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&indexBuffer)));

        UINT8* pIndexDataBegin;
        CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
        memcpy(pIndexDataBegin, mesh->indices.data(), indexBufferSize);
        indexBuffer->Unmap(0, nullptr);
		indexBuffers.push_back(indexBuffer);

		D3D12_INDEX_BUFFER_VIEW indexBufferView{
			indexBuffer->GetGPUVirtualAddress(),
			indexBufferSize,
			DXGI_FORMAT_R32_UINT
		};
		indexBufferViews.push_back(indexBufferView);
    }

    // Create model matrix
    {
		const UINT constantBufferSize = sizeof(DirectX::XMFLOAT4X4);

        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);
		ComPtr<ID3D12Resource> constantBuffer;
        ThrowIfFailed(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&constantBuffer)));

        // Map and initialize the constant buffer
        UINT8* pVertexDataBegin;
        DirectX::XMFLOAT4X4* pData;
        constantBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pData));
        DirectX::XMStoreFloat4x4(pData, meshTransform);
        constantBuffer->Unmap(0, nullptr);

		this->transformCBs.push_back(constantBuffer);
    }

	++meshCount;
}

void MeshRenderer::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList, const ComPtr<ID3D12DescriptorHeap>& srtHeap, const Microsoft::WRL::ComPtr<ID3D12Device>& device) const
{
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (int i = 0; i < meshCount; ++i) {
        
        //commandList->SetGraphicsRootConstantBufferView(2, this->transformCBs[i]->GetGPUVirtualAddress());

        auto e_model = EngineMain::instance->m_ModelMatrix;
		auto e_view = EngineMain::instance->m_ViewMatrix;
		auto e_projection = EngineMain::instance->m_ProjectionMatrix;
		auto result = XMMatrixMultiply(( e_model ), (this->meshTransforms[i]));
		auto mvpMatrix = XMMatrixMultiply(result, e_view);
		mvpMatrix = XMMatrixMultiply(mvpMatrix, e_projection);

		EngineMain::instance->dxRenderManager->SetModelMatrix(commandList, mvpMatrix);
        commandList->IASetVertexBuffers(0, 1, &vertexBufferViews[i]);
        commandList->IASetIndexBuffer(&indexBufferViews[i]);
        //commandList->SetGraphicsRootDescriptorTable(1, srtHeap->GetGPUDescriptorHandleForHeapStart()); // Diffuse map
        //commandList->SetGraphicsRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(srtHeap->GetGPUDescriptorHandleForHeapStart(), 1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV))); // Normal map
        commandList->DrawIndexedInstanced(this->meshes[i]->indices.size(), 1, 0, 0, 0);
    }

    EngineMain::instance->dxRenderManager->ResetModelMatrix(commandList);
}
