#include "DXSpriteRenderer.h"

#include <d3dx12.h>

#include "Graphics/Texture.h"
#include "PlatformHelpers.h"

DXSpriteRenderer::DXSpriteRenderer(): x(0), y(0), width(0), height(0), m_vertexBufferView()
{
}

DXSpriteRenderer::~DXSpriteRenderer()
{
}

void DXSpriteRenderer::Start(float x, float y, float width, float height, const char* texturePath, const ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12DescriptorHeap> srvHeap)
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

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
    // the command list that references it has finished executing on the GPU.
    // We will flush the GPU at the end of this method to ensure the resource is not
    // prematurely destroyed.
    // ComPtr<ID3D12Resource> textureUploadHeap;

    // Create the texture.
    {
        texture = Texture::LoadFromFile(texturePath);

        // Describe and create a Texture2D.
        D3D12_RESOURCE_DESC textureDesc = {};
        textureDesc.MipLevels = 1;
        textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        textureDesc.Width = texture->GetWidth();
        textureDesc.Height = texture->GetHeight();
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

        // const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_texture.Get(), 0, 1);

        // Get the required size for the upload heap
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferDesc;
		UINT64 textureUploadBufferSize;
		device->GetCopyableFootprints(
		    &textureDesc,
		    0, // First subresource
		    1, // Number of subresources
		    0, // Base offset
		    &bufferDesc,
		    nullptr,
		    nullptr,
		    &textureUploadBufferSize
		);

        // Create the GPU upload buffer.
        hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        auto uploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize);
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
        textureData.RowPitch = texture->GetWidth() * 4;
        textureData.SlicePitch = textureData.RowPitch * texture->GetHeight();

        // UpdateSubresources(commandList.Get(), m_texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
        UINT8* pData;
        // auto textureDataSize = texture->GetWidth() * texture->GetHeight() * TexturePixelSize;
        textureUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pData));
		memcpy(pData, rawData, textureData.SlicePitch);
		textureUploadHeap->Unmap(0, nullptr);
        
  //       for (size_t i = 0; i < texture->GetWidth() * texture->GetHeight(); ++i) {
		// 	printf("Pixel %zu: R = %u, G = %u, B = %u, A = %u\n", i, 
		// 	texture->GetData()[i * 4 + 0], 
		// 	texture->GetData()[i * 4 + 1], 
		// 	texture->GetData()[i * 4 + 2], 
		// 	texture->GetData()[i * 4 + 3]);
		// }

        // Record commands to copy data from upload heap to texture
		D3D12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(m_texture.Get(), 0);
		D3D12_TEXTURE_COPY_LOCATION src = CD3DX12_TEXTURE_COPY_LOCATION(textureUploadHeap.Get(), bufferDesc);
		commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

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

void DXSpriteRenderer::Render(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->DrawInstanced(6, 1, 0, 0);
}
