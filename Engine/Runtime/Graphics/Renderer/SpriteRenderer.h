#pragma once

#include "Runtime/EngineIncludes.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "Runtime/Graphics/Texture.h"
//#include "Core/Component.h"
#include "Runtime/Graphics/Renderer/Renderer.h"

DELTA_ENGINE_NS_BEGIN

class SpriteRenderer: public Renderer
{
public:
	SpriteRenderer();
	~SpriteRenderer();

	void Start(float width, float height, const char* texturePath, Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap);
	void Render(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& commandList) const;

private:
	float width;
	float height;
    const char* m_texturePath;
	
    // ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    // ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	// Index buffer for the cube.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_texture;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureUploadHeap;
	Texture *texture;

	struct Vertex
    {
	    DirectX::XMFLOAT3 position;
	    DirectX::XMFLOAT4 color;
	    DirectX::XMFLOAT2 uv;
    };

protected:
    virtual void InitGraphicState(DXGraphicsContext context) override;
    virtual void GatherDrawCalls(DXGraphicsContext context) override;

private:
    // static const UINT FrameCount = 2;
    // static const UINT TextureWidth = 256;
    // static const UINT TextureHeight = 256;
    static const UINT TexturePixelSize = 4;    // The number of bytes used to represent a pixel in the texture.
};

DELTA_ENGINE_NS_END
