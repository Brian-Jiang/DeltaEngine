#pragma once

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "Graphics/Texture.h"
#include "Core/Component.h"

namespace DeltaEngine
{

class SpriteRenderer: public Component
{
	public:
	SpriteRenderer();
	~SpriteRenderer();

	void Start(float x, float y, float width, float height, const char* texturePath, Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap);
	void Render(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) const;

private:
	float x;
	float y;
	float width;
	float height;
	
    // ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    // ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureUploadHeap;
	Texture *texture;

	struct Vertex
    {
	    DirectX::XMFLOAT3 position;
	    DirectX::XMFLOAT4 color;
	    DirectX::XMFLOAT2 uv;
    };

private:
    // static const UINT FrameCount = 2;
    // static const UINT TextureWidth = 256;
    // static const UINT TextureHeight = 256;
    static const UINT TexturePixelSize = 4;    // The number of bytes used to represent a pixel in the texture.
};

}
