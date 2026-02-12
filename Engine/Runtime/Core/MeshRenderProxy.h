#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <d3d12.h>
#include <DirectXCollision.h>

DELTA_ENGINE_NS_BEGIN

class PipelineStateObject;
class RootSignature;
struct DXGraphicsContext;
class DMesh;
struct MeshRendererSettings;
class IndexBuffer;
class VertexBuffer;

class MeshRenderProxy
{
public:
    MeshRenderProxy();
    MeshRenderProxy(std::shared_ptr<DMesh> mesh, std::shared_ptr<MeshRendererSettings> settings);
    ~MeshRenderProxy();

    void SetMesh(std::shared_ptr<DMesh> mesh);
    
    void BuildPipelineStateObject(std::shared_ptr<DXGraphicsContext> renderContext);
    void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext);

    /**
    * Get the number if indices in the index buffer.
    * If no index buffer is bound to the mesh, this function returns 0.
    */
    size_t GetIndexCount() const;

    /**
    * Get the number of vertices in the mesh.
    * If this mesh does not have a vertex buffer, the function returns 0.
    */
    size_t GetVertexCount() const;

private:
    std::shared_ptr<DMesh> m_mesh;
    std::shared_ptr<MeshRendererSettings> m_settings;

    using BufferMap = std::map<uint32_t, std::shared_ptr<VertexBuffer>>;
    BufferMap m_VertexBuffers;
    std::shared_ptr<IndexBuffer> m_IndexBuffer;
    D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
    DirectX::BoundingBox m_AABB;

    // todo: use a pso manager to manage PSOs and avoid creating a PSO for each mesh render proxy.
    // maybe similar to srp batcher, we can have a pso batcher that batches mesh render proxies with the same settings and creates a PSO for each batch.
    std::shared_ptr<PipelineStateObject> m_pipelineStateObject;

    bool m_meshDirty;
};

DELTA_ENGINE_NS_END
