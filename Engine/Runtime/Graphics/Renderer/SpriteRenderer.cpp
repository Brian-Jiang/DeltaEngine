#include "SpriteRenderer.h"

#include <d3dx12.h>

#include "Graphics/Texture.h"
// #include "PlatformHelpers.h"
#include "Graphics/DXUtils.h"
#include "EngineMain.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace DeltaEngine;

// class EngineMain;

SpriteRenderer::SpriteRenderer(): x(0), y(0), width(0), height(0), vertexBufferView(), texture(nullptr)
{
}

SpriteRenderer::~SpriteRenderer()
{
}

void SpriteRenderer::Start(float x, float y, float width, float height, const char* texturePath, const ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12DescriptorHeap> srvHeap)
{
    this->x = x;
	this->y = y;
	this->width = width;
	this->height = height;

	// Create the vertex buffer.
    {
        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { x, y + height, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
			{ { x + width, y, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
            { { x, y, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
			{ { x, y + height, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
			{ { x + width, y + height, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
			{ { x + width, y, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
        };

         const UINT vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
         CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
         auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
         ThrowIfFailed(device->CreateCommittedResource(
             &heapProps,
             D3D12_HEAP_FLAG_NONE,
             &desc,
             D3D12_RESOURCE_STATE_GENERIC_READ,
             nullptr,
             IID_PPV_ARGS(&m_vertexBuffer)));

        //auto dxRenderManager = EngineMain::instance->dxRenderManager;
        //ComPtr<ID3D12Resource> intermediateVertexBuffer;
        //DXUtils::UpdateBufferResource(
        //    dxRenderManager->GetDevice(),
        //    dxRenderManager->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetCommandList(),
        //    &m_vertexBuffer,
        //    &intermediateVertexBuffer,
        //    _countof(triangleVertices),
        //    sizeof(Vertex),
        //    triangleVertices);

        // Copy the triangle data to the vertex buffer.
         UINT8* pVertexDataBegin;
         CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
         ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
         memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
         m_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        vertexBufferView.StrideInBytes = sizeof(Vertex);
        vertexBufferView.SizeInBytes = sizeof(triangleVertices);
    }

    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    // ComPtr<ID3D12Resource> textureUploadHeap;

    // Create the texture.
    {
        texture = Texture::LoadFromFile(texturePath);
        auto textureHeight = texture->GetHeight();
        auto textureWidth = texture->GetWidth();

        // Describe and create a Texture2D.
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
        ThrowIfFailed(device->CreateCommittedResource(
            &hp,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_texture)));
        
        UINT64 rowPitch = textureWidth * TexturePixelSize;
        UINT64 alignedRowPitch = (rowPitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
        // UINT64 alignedRowPitch = Align(texture->GetWidth() * sizeof(DWORD), D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
        UINT64 textureSize = alignedRowPitch * textureHeight;

        // auto dxRenderManager = EngineMain::instance->dxRenderManager;
        

        // Create the GPU upload buffer.
        hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto uploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(textureSize);
        ThrowIfFailed(device->CreateCommittedResource(
            &hp,
            D3D12_HEAP_FLAG_NONE,
            &uploadHeapDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&textureUploadHeap)));

        // Copy data to the intermediate upload heap and then schedule a copy 
        // from the upload heap to the Texture2D.
        

        auto rawData = texture->GetData().data();
        D3D12_SUBRESOURCE_DATA textureData = {};
        textureData.pData = rawData;
        textureData.RowPitch = alignedRowPitch;
        textureData.SlicePitch = alignedRowPitch * textureHeight;

        D3D12_SUBRESOURCE_FOOTPRINT pitchedDesc = { };
		pitchedDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		pitchedDesc.Width = textureWidth;
		pitchedDesc.Height = textureHeight;
		pitchedDesc.Depth = 1;
		pitchedDesc.RowPitch = alignedRowPitch;
        
        UINT8* pData;
        // auto textureDataSize = texture->GetWidth() * texture->GetHeight() * TexturePixelSize;
        textureUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pData));
		// memcpy(pData, rawData, textureData.SlicePitch);
        for (int y = 0; y < textureHeight; y++) {
		    memcpy(pData + y * alignedRowPitch, rawData + y * rowPitch, rowPitch);
		}
		textureUploadHeap->Unmap(0, nullptr);

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTexture2D = { 0 };
		placedTexture2D.Offset = 0;
		placedTexture2D.Footprint = pitchedDesc;

        // Record commands to copy data from upload heap to texture
		D3D12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(m_texture.Get(), 0);
		D3D12_TEXTURE_COPY_LOCATION src = CD3DX12_TEXTURE_COPY_LOCATION(textureUploadHeap.Get(), placedTexture2D);
		commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        // UpdateSubresources(commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);

        auto rb = CD3DX12_RESOURCE_BARRIER::Transition(
            m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    	commandList->ResourceBarrier(1, &rb);

        // Describe and create a SRV for the texture.
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(m_texture.Get(), &srvDesc, srvHeap->GetCPUDescriptorHandleForHeapStart());
    }
}

void SpriteRenderer::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
    commandList->DrawInstanced(6, 1, 0, 0);
}
