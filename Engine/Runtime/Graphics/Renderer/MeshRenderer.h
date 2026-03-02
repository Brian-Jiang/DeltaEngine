#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include "Runtime/Core/DTexture.h"
#include "Runtime/Graphics/Renderer/Renderer.h"

#include "MeshRenderer.generated.h"

DELTA_ENGINE_NS_BEGIN

class EngineMain;
class MeshRenderProxy;
class DMesh;

struct MeshRendererSettings
{
    // Future settings for mesh rendering (e.g., culling mode, shadow casting, etc.) can be added here.
};

DCLASS()
class MeshRenderer: public Renderer
{
    DGENERATED_BODY(MeshRenderer)

public:
    MeshRenderer();
    MeshRenderer(std::string name);
    MeshRenderer(std::string name, std::shared_ptr<GameObject> gameObject);
    ~MeshRenderer();

    DFUNCTION()
    void SetMesh(std::shared_ptr<DMesh> mesh);

protected:
    void InitGraphicState(std::shared_ptr<DXGraphicsContext> context) override;
    void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> context) override;
    void OnTransformChanged() override;

private:
    //void LoadTexture(const std::shared_ptr<DTexture>& texture, DXGraphicsContext& context);
    //void AddMesh(const Mesh* mesh, const DirectX::XMMATRIX meshTransform, DXGraphicsContext& context);

    void CreateMeshRenderProxy();


private:
    //std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferViews;
    //std::vector<D3D12_INDEX_BUFFER_VIEW> indexBufferViews;

    DPROPERTY()
    std::vector<std::shared_ptr<DTexture>> loadedTextures;

    //Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

    /// Per-object constant buffer (world matrix + color), root parameter [2].
    //Microsoft::WRL::ComPtr<ID3D12Resource> m_objectCb;

	//std::vector<Mesh*> meshes;
    DPROPERTY()
    std::vector<DirectX::XMMATRIX> meshTransforms;

    DPROPERTY()
    std::shared_ptr<DMesh> m_mesh;

    DPROPERTY()
    std::shared_ptr<MeshRenderProxy> m_meshRenderProxy;
    DPROPERTY()
    MeshRendererSettings m_settings;

    DPROPERTY()
	int loadedTextureCount;
    DPROPERTY()
	int meshCount;

    DPROPERTY()
    bool m_dirty;
};

DELTA_ENGINE_NS_END
