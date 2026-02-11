#pragma once

#include "EngineIncludes.h"

#include <vector>
#include <string>
#include <memory>
#include <d3d12.h>
#include <DirectXCollision.h>

DELTA_ENGINE_NS_BEGIN

class PipelineStateObject;
class RootSignature;
struct DXGraphicsContext;
class DMesh;
class MeshRendererSettings;
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

    // An enum for root signature parameters.
    // I'm not using scoped enums to avoid the explicit cast that would be required
    // to use these as root indices in the root signature.
    enum RootParameters
    {
        // Vertex shader parameter
        MatricesCB, // ConstantBuffer<Matrices> MatCB : register(b0);

        // Pixel shader parameters
        MaterialCB, // ConstantBuffer<Material> MaterialCB : register( b0, space1 );
        LightPropertiesCB, // ConstantBuffer<LightProperties> LightPropertiesCB : register( b1 );

        PointLights, // StructuredBuffer<PointLight> PointLights : register( t0 );
        SpotLights, // StructuredBuffer<SpotLight> SpotLights : register( t1 );
        DirectionalLights, // StructuredBuffer<DirectionalLight> DirectionalLights : register( t2 )

        Textures, // Texture2D AmbientTexture       : register( t3 );
                  // Texture2D EmissiveTexture : register( t4 );
                  // Texture2D DiffuseTexture : register( t5 );
                  // Texture2D SpecularTexture : register( t6 );
                  // Texture2D SpecularPowerTexture : register( t7 );
                  // Texture2D NormalTexture : register( t8 );
                  // Texture2D BumpTexture : register( t9 );
                  // Texture2D OpacityTexture : register( t10 );
        NumRootParameters
    };

    // Light properties for the pixel shader.
    struct LightProperties
    {
        uint32_t NumPointLights;
        uint32_t NumSpotLights;
        uint32_t NumDirectionalLights;
    };

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
    std::shared_ptr<RootSignature> m_rootSignature;
    std::shared_ptr<PipelineStateObject> m_pipelineStateObject;

    bool m_meshDirty;
};

DELTA_ENGINE_NS_END
