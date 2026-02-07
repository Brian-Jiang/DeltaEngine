#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "Graphics/Texture.h"
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
    void LoadTexture(const Texture* texture, const DXGraphicsContext& context, UINT descriptorIndex);
    void AddMesh(const Mesh* mesh, const DirectX::XMMATRIX meshTransform, const DXGraphicsContext& context);

    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews;
    std::vector<D3D12_INDEX_BUFFER_VIEW> indexBufferViews;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textureResources;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textureUploadResources;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	std::vector<Mesh*> meshes;
    std::vector<DirectX::XMMATRIX> meshTransforms;

	int loadedTextureCount;
	int meshCount;

    static const UINT TexturePixelSize = 4;
};

DELTA_ENGINE_NS_END
