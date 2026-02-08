#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "Runtime/Graphics/DTexture.h"
#include "Runtime/Graphics/Renderer/Renderer.h"
#include "Runtime/Graphics/Mesh.h"

DELTA_ENGINE_NS_BEGIN

class EngineMain;

class MeshRenderer: public Renderer
{
public:
    MeshRenderer();
    ~MeshRenderer();

    /// Store mesh data. Actual GPU resource creation happens in InitGraphicState.
    void Start(const std::vector<Mesh*>& meshes, const std::vector<DirectX::XMMATRIX>& meshTransforms);

protected:
    void InitGraphicState(DXGraphicsContext& context) override;
    void GatherDrawCalls(DXGraphicsContext& context) override;

private:
    void LoadTexture(const std::shared_ptr<DTexture>& texture, DXGraphicsContext& context);
    void AddMesh(const Mesh* mesh, const DirectX::XMMATRIX meshTransform, DXGraphicsContext& context);

    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews;
    std::vector<D3D12_INDEX_BUFFER_VIEW> indexBufferViews;

    std::vector<std::shared_ptr<DTexture>> loadedTextures;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

    /// Per-object constant buffer (world matrix + color), root parameter [2].
    Microsoft::WRL::ComPtr<ID3D12Resource> m_objectCb;

	std::vector<Mesh*> meshes;
    std::vector<DirectX::XMMATRIX> meshTransforms;

	int loadedTextureCount;
	int meshCount;
};

DELTA_ENGINE_NS_END
