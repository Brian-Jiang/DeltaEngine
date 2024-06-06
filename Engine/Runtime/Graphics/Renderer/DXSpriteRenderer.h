#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "Graphics/Texture.h"

using namespace DirectX;
using namespace Microsoft::WRL;

class DXSpriteRenderer
{
	public:
	DXSpriteRenderer();
	~DXSpriteRenderer();

	void Start(float x, float y, float width, float height, const char* texturePath, ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> commandList, ComPtr<ID3D12DescriptorHeap> srvHeap);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& commandList);

private:
	float x;
	float y;
	float width;
	float height;
	// unsigned int vbo;
	// unsigned int VAO;
	// unsigned int EBO;
	
    // ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    // ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_texture;
	ComPtr<ID3D12Resource> textureUploadHeap;
	Texture *texture;

	struct Vertex
    {
	    XMFLOAT3 position;
	    XMFLOAT4 color;
	    XMFLOAT2 uv;
    };

private:
    // static const UINT FrameCount = 2;
    // static const UINT TextureWidth = 256;
    // static const UINT TextureHeight = 256;
    static const UINT TexturePixelSize = 4;    // The number of bytes used to represent a pixel in the texture.
};

