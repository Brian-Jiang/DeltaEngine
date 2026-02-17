#include "Runtime/Graphics/RenderProxy/MeshRenderProxy.h"

#include <d3dx12.h>
#include <dxcapi.h>

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/DirectX/Device.h"
#include "Graphics/DirectX/RootSignature.h"
#include "Graphics/DirectX/CommandQueue.h"
#include "Graphics/DirectX/CommandList.h"
#include "Graphics/DirectX/IndexBuffer.h"
#include "Graphics/DirectX/VertexBuffer.h"
#include "Graphics/Structures/Camera.h"
#include "Graphics/Structures/Light.h"
#include "Graphics/DXRenderManager.h"
#include "Graphics/Structures/RootParameterType.h"
#include "Core/DMesh.h"
#include "Core/DMaterial.h"
#include "Core/DShader.h"

using namespace DeltaEngine;

MeshRenderProxy::MeshRenderProxy()
    : m_PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
{
}

DeltaEngine::MeshRenderProxy::MeshRenderProxy(std::shared_ptr<DMesh> mesh, std::shared_ptr<MeshRendererSettings> settings)
    : m_mesh(mesh)
    , m_settings(settings)
    , m_PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) // todo : get primitive topology from mesh
{
}

DeltaEngine::MeshRenderProxy::~MeshRenderProxy()
{
}

void DeltaEngine::MeshRenderProxy::SetMesh(std::shared_ptr<DMesh> mesh)
{
    m_mesh = mesh;
    m_meshDirty = true;
}

void DeltaEngine::MeshRenderProxy::UpdateWorldTransform(DirectX::XMMATRIX worldMatrix)
{
    m_worldMatrix = worldMatrix;
}

void DeltaEngine::MeshRenderProxy::BuildPipelineStateObject(std::shared_ptr<DXGraphicsContext> renderContext)
{
    std::shared_ptr<Device> device = renderContext->device;

    // Setup the pipeline state.
    struct PipelineStateStream {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RasterizerState;
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BlendState;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencilState;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
    } pipelineStateStream;

    // Create a color buffer with sRGB for gamma correction.
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

    // Check the best multisample quality level that can be used for the given back buffer format.
    DXGI_SAMPLE_DESC sampleDesc = device->GetMultisampleQualityLevels(backBufferFormat);

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = backBufferFormat;

    CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);
    //if (m_EnableDecal) {
    //    // Disable backface culling on decal geometry.
    //    rasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    //}

    m_pipelineStateObjects.clear();
    for (int i = 0; i < m_mesh->GetSubMeshCount(); ++i)
    {
        auto material = m_mesh->GetMaterial(i);
        CD3DX12_BLEND_DESC blendDesc = material->GetBlendState();
        CD3DX12_DEPTH_STENCIL_DESC depthStencilState = material->GetDepthStencilState();

        IDxcBlob* vertexShader = material->GetShader()->GetVertexShaderBlob();
        CD3DX12_SHADER_BYTECODE vertexShaderBytecode { static_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };

        IDxcBlob* pixelShader = material->GetShader()->GetPixelShaderBlob();
        CD3DX12_SHADER_BYTECODE pixelShaderBytecode { static_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };

        std::vector<D3D12_INPUT_ELEMENT_DESC> layout = material->GetShader()->GetInputLayout();
        pipelineStateStream.InputLayout = { layout.data(), static_cast<UINT>(layout.size()) };
        pipelineStateStream.pRootSignature = renderContext->renderManager->GetRootSignature()->GetD3D12RootSignature().Get();
        pipelineStateStream.VS = vertexShaderBytecode;
        pipelineStateStream.PS = pixelShaderBytecode;
        pipelineStateStream.RasterizerState = rasterizerState;
        pipelineStateStream.BlendState = blendDesc;
        pipelineStateStream.DepthStencilState = depthStencilState;
        pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pipelineStateStream.DSVFormat = depthBufferFormat;
        pipelineStateStream.RTVFormats = rtvFormats;
        pipelineStateStream.SampleDesc = sampleDesc;

        m_pipelineStateObjects.push_back(device->CreatePipelineStateObject(pipelineStateStream));

        if (material)
        {
            m_textures.insert({ i, std::unordered_map<uint32_t, std::shared_ptr<DirectX12Texture>>() });
            auto texture = material->GetTexture(0);
            if (texture)
            {
                m_textures[i][0] = renderContext->commandList->LoadTexture(texture);
            }
        }
    }
}

void MeshRenderProxy::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
    std::shared_ptr<CommandList> commandList = renderContext->commandList;

    if (m_meshDirty)
    {
        int submeshCount = m_mesh->GetSubMeshCount();
        for (int i = 0; i < submeshCount; ++i)
        {
            std::shared_ptr<VertexBuffer> vertexBuffer = commandList->CopyVertexBuffer(m_mesh->GetVertices()[i]);
            m_VertexBuffers.push_back(vertexBuffer);
            std::shared_ptr<IndexBuffer> indexBuffer = commandList->CopyIndexBuffer(m_mesh->GetIndices()[i]);
            m_IndexBuffers.push_back(indexBuffer);
            
            // m_PrimitiveTopology = m_mesh->GetPrimitiveTopology();
            // m_AABB = m_mesh->GetAABB();
        }
        
        BuildPipelineStateObject(renderContext);

        m_meshDirty = false;
    }

    // todo where to set object cb?
    struct ObjectData
    {
        DirectX::XMMATRIX worldMatrix;
        DirectX::XMFLOAT4 color;
        uint32_t useInstanceMatrix;
    } obj;

    obj.worldMatrix = m_worldMatrix;
    obj.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    obj.useInstanceMatrix = 0;

    commandList->SetGraphicsDynamicConstantBuffer(static_cast<UINT>(RootParameterType::ObjectCB), obj);

    for (int i = 0; i < m_pipelineStateObjects.size(); ++i)
    {
        commandList->SetPipelineState(m_pipelineStateObjects[i]);
        commandList->SetPrimitiveTopology(m_PrimitiveTopology);

        
        commandList->SetShaderResourceView(static_cast<uint32_t>(RootParameterType::Texture), 0, m_textures[i][0],
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        //for (auto vertexBuffer : m_VertexBuffers)
        //{
        //    commandList->SetVertexBuffer(0, vertexBuffer);
        //}

        commandList->SetVertexBuffer(0, m_VertexBuffers[i]);

        auto indexCount = m_IndexBuffers[i]->GetNumIndices();
        auto vertexCount = m_VertexBuffers[i]->GetNumVertices();

        if (indexCount > 0) {
            commandList->SetIndexBuffer(m_IndexBuffers[i]);
            commandList->DrawIndexed(indexCount, 1u, 0u, 0u, 0u);
        } else if (vertexCount > 0) {
            commandList->Draw(vertexCount, 1u, 0u, 0u);
        }
    }
}

size_t DeltaEngine::MeshRenderProxy::GetIndexCount() const
{
    size_t indexCount = 0;
    if (!m_IndexBuffers.empty()) {
        indexCount = m_IndexBuffers[0]->GetNumIndices();
    }

    return indexCount;
}

size_t DeltaEngine::MeshRenderProxy::GetVertexCount() const
{
    size_t vertexCount = 0;

    // To count the number of vertices in the mesh, just take the number of vertices in the first vertex buffer.
    if (!m_VertexBuffers.empty()) {
        vertexCount = m_VertexBuffers[0]->GetNumVertices();
    }

    return vertexCount;
}
