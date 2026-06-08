#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <memory>
#include <unordered_map>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class PipelineStateObject;
class DMesh;
struct MeshRendererSettings;
class IndexBuffer;
class VertexBuffer;
class DirectX12Texture;
class DMaterial;

class MeshRenderProxy : public RenderProxy
{
public:
    DELTAENGINE_API MeshRenderProxy();
    DELTAENGINE_API MeshRenderProxy(DMesh* mesh, std::shared_ptr<MeshRendererSettings> settings);
    DELTAENGINE_API ~MeshRenderProxy() override;

    /// Replaces the mesh used by the render proxy.
    DELTAENGINE_API void SetMesh(DMesh* mesh);

    /// Updates the world transform used for the next draw submission.
    DELTAENGINE_API void UpdateWorldTransform(DirectX::XMMATRIX worldMatrix);

    /// Builds pipeline state objects and uploads textures for the current mesh.
    DELTAENGINE_API void Initialize(std::shared_ptr<DXGraphicsContext> renderContext) override;

    /// Records draw calls for the current mesh into the active command list.
    DELTAENGINE_API void GatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext) override;

    DELTAENGINE_API void GatherShadowDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext, const ShadowView& view) override;

    DELTAENGINE_API static bool SubmeshContributesToShadowMap(const DMaterial* material);

    /// Returns the number of indices in the first index buffer.
    DELTAENGINE_API size_t GetIndexCount() const;

    /// Returns the number of vertices in the first vertex buffer.
    DELTAENGINE_API size_t GetVertexCount() const;

    DELTAENGINE_API bool HasExclusiveGPUResources() const override;
    DELTAENGINE_API void ReleaseSharedReferences() override;

private:
    DMesh* m_mesh;
    std::shared_ptr<MeshRendererSettings> m_settings;

    std::vector<std::shared_ptr<VertexBuffer>> m_VertexBuffers;
    std::vector<std::shared_ptr<IndexBuffer>> m_IndexBuffers;
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, std::shared_ptr<DirectX12Texture>>> m_textures;
    D3D12_PRIMITIVE_TOPOLOGY m_PrimitiveTopology;
    DirectX::XMMATRIX m_worldMatrix;

    // todo: use a pso manager to manage PSOs and avoid creating a PSO for each mesh render proxy.
    // maybe similar to srp batcher, we can have a pso batcher that batches mesh render proxies with the same settings and creates a PSO for each batch.
    std::vector<std::shared_ptr<PipelineStateObject>> m_pipelineStateObjects;

    bool m_meshDirty;
};

DELTA_ENGINE_NS_END
