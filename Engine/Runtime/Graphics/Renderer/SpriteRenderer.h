#pragma once

#include "Runtime/EngineIncludes.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "Runtime/Core/DTexture.h"
#include "Runtime/Graphics/Renderer/Renderer.h"

DELTA_ENGINE_NS_BEGIN

class SpriteRenderer: public Renderer
{
public:
	SpriteRenderer();
	~SpriteRenderer();

	void Start(float width, float height, const char* texturePath);

private:
	float width;
	float height;
    const char* m_texturePath;
	
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
	// Index buffer for the cube.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	std::shared_ptr<DTexture> m_texture;

	// Per-renderer pipeline state
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	/// Per-object constant buffer (world matrix + color), root parameter [2].
	Microsoft::WRL::ComPtr<ID3D12Resource> m_objectCb;

	struct Vertex
    {
	    DirectX::XMFLOAT3 position;
	    DirectX::XMFLOAT4 color;
	    DirectX::XMFLOAT3 normal;
	    DirectX::XMFLOAT2 uv;
    };

protected:
    virtual void InitGraphicState(DXGraphicsContext& context) override;
    virtual void GatherDrawCalls(DXGraphicsContext& context) override;

};

DELTA_ENGINE_NS_END
