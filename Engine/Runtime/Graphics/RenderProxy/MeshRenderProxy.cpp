#include "Runtime/Graphics/RenderProxy/MeshRenderProxy.h"

#include <d3dx12.h>
#include <dxcapi.h>

#include <filesystem>

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/DXRenderManager.h"
#include "Graphics/DefaultTextures.h"
#include "Graphics/DirectX/CommandList.h"
#include "Graphics/DirectX/Device.h"
#include "Graphics/DirectX/DirectX12Texture.h"
#include "Graphics/DirectX/IndexBuffer.h"
#include "Graphics/DirectX/PipelineStateObject.h"
#include "Graphics/DirectX/RootSignature.h"
#include "Graphics/DirectX/VertexBuffer.h"
#include "Graphics/MaterialConstants.h"
#include "Graphics/Structures/RootParameterType.h"
#include "Graphics/Shadow/ShadowDepthPSO.h"
#include "Graphics/Shadow/ShadowView.h"
#include "Core/DMaterial.h"
#include "Core/DMesh.h"
#include "Core/DShader.h"
#include "Core/DTexture.h"

using namespace DeltaEngine;
using namespace DirectX;

MeshRenderProxy::MeshRenderProxy()
    : m_mesh(nullptr)
    , m_PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
    , m_worldMatrix(DirectX::XMMatrixIdentity())
    , m_meshDirty(true)
{
}

DeltaEngine::MeshRenderProxy::MeshRenderProxy(DMesh* mesh, std::shared_ptr<MeshRendererSettings> settings)
    : m_mesh(mesh)
    , m_settings(settings)
    , m_PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) // todo : get primitive topology from mesh
    , m_worldMatrix(DirectX::XMMatrixIdentity())
    , m_meshDirty(true)
{
}

DeltaEngine::MeshRenderProxy::~MeshRenderProxy()
{
}

void DeltaEngine::MeshRenderProxy::SetMesh(DMesh* mesh)
{
    if (mesh != m_mesh)
    {
        const std::string newName = mesh ? mesh->GetSourcePath().stem().string() : std::string("(null)");
        const std::string oldName = m_mesh ? m_mesh->GetSourcePath().stem().string() : std::string("(null)");
        DLOG(LogRenderer, ELogLevel::Verbose, "MeshRenderProxy::SetMesh '{}' -> '{}'", oldName, newName);
    }

    m_mesh = mesh;
    m_VertexBuffers.clear();
    m_IndexBuffers.clear();
    m_textures.clear();
    m_pipelineStateObjects.clear();
    m_meshDirty = true;
}

void DeltaEngine::MeshRenderProxy::UpdateWorldTransform(DirectX::XMMATRIX worldMatrix)
{
    m_worldMatrix = worldMatrix;
}

void DeltaEngine::MeshRenderProxy::Initialize(std::shared_ptr<DXGraphicsContext> renderContext)
{
    if (!DELTA_ENSURE(m_mesh))
    {
        DLOG(LogRenderer, ELogLevel::Warning, "MeshRenderProxy::Initialize skipped: mesh is null");
        return;
    }
    if (!DELTA_ENSURE(renderContext))
    {
        DLOG(LogRenderer, ELogLevel::Warning, "MeshRenderProxy::Initialize skipped: renderContext is null");
        return;
    }
    if (!DELTA_ENSURE(renderContext->device && renderContext->commandList && renderContext->renderManager))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "MeshRenderProxy::Initialize skipped: renderContext is missing device/commandList/renderManager");
        return;
    }
    if (!DELTA_ENSURE(renderContext->renderManager->GetRootSignature()))
    {
        DLOG(LogRenderer, ELogLevel::Warning, "MeshRenderProxy::Initialize skipped: root signature is null");
        return;
    }

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

    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
    DXGI_SAMPLE_DESC sampleDesc = device->GetMultisampleQualityLevels(backBufferFormat);

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = backBufferFormat;

    m_textures.clear();
    m_pipelineStateObjects.clear();
    const std::string meshName = m_mesh->GetSourcePath().stem().string();
    for (int i = 0; i < m_mesh->GetSubMeshCount(); ++i)
    {
        auto material = m_mesh->GetMaterial(i);
        if (!DELTA_ENSURE(material))
        {
            DLOG(LogRenderer, ELogLevel::Warning,
                "MeshRenderProxy::Initialize: submesh {} of '{}' has null material; skipping submesh",
                i, meshName);
            m_pipelineStateObjects.push_back(nullptr);
            continue;
        }
        if (!DELTA_ENSURE(material->GetShader()))
        {
            DLOG(LogRenderer, ELogLevel::Warning,
                "MeshRenderProxy::Initialize: submesh {} of '{}' has material with null shader; skipping submesh",
                i, meshName);
            m_pipelineStateObjects.push_back(nullptr);
            continue;
        }

        ISlangBlob* vertexShader = material->GetShader()->GetVertexShaderBlob();
        ISlangBlob* pixelShader = material->GetShader()->GetPixelShaderBlob();
        if (!DELTA_ENSURE(vertexShader && pixelShader))
        {
            DLOG(LogRenderer, ELogLevel::Warning,
                "MeshRenderProxy::Initialize: submesh {} of '{}' has shader missing VS or PS blob; skipping submesh",
                i, meshName);
            m_pipelineStateObjects.push_back(nullptr);
            continue;
        }

        std::vector<D3D12_INPUT_ELEMENT_DESC> layout = material->GetShader()->GetInputLayout();
        if (!DELTA_ENSURE(!layout.empty()))
        {
            DLOG(LogRenderer, ELogLevel::Warning,
                "MeshRenderProxy::Initialize: submesh {} of '{}' has empty shader input layout; skipping submesh",
                i, meshName);
            m_pipelineStateObjects.push_back(nullptr);
            continue;
        }

        CD3DX12_BLEND_DESC blendDesc = material->GetBlendState();
        CD3DX12_DEPTH_STENCIL_DESC depthStencilState = material->GetDepthStencilState();

        CD3DX12_SHADER_BYTECODE vertexShaderBytecode { const_cast<void*>(vertexShader->getBufferPointer()), vertexShader->getBufferSize() };
        CD3DX12_SHADER_BYTECODE pixelShaderBytecode { const_cast<void*>(pixelShader->getBufferPointer()), pixelShader->getBufferSize() };

        CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);
        if (HasAny(material->GetFlags(), MaterialFlags::DoubleSided))
            rasterizerState.CullMode = D3D12_CULL_MODE_NONE;

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
        if (m_pipelineStateObjects.back())
        {
            m_pipelineStateObjects.back()->GetD3D12PipelineState()->SetName(
                (L"PSO MeshRenderProxy Sub" + std::to_wstring(i)).c_str());
        }

        m_textures.insert({ i, std::unordered_map<uint32_t, std::shared_ptr<DirectX12Texture>>() });
        const uint32_t slotCount = static_cast<uint32_t>(MaterialTextureSlot::Count);
        for (uint32_t slot = 0; slot < slotCount; ++slot)
        {
            DTexture* tex = material->GetTexture(static_cast<int>(slot));
            if (tex)
                m_textures[i][slot] = renderContext->commandList->LoadTexture(tex);
        }
    }
}

void MeshRenderProxy::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
    if (!m_mesh)
        return;

    if (!DELTA_ENSURE(renderContext && renderContext->commandList))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "MeshRenderProxy::GatherDrawCalls skipped: renderContext or commandList is null");
        return;
    }

    std::shared_ptr<CommandList> commandList = renderContext->commandList;

    if (m_meshDirty)
    {
        m_VertexBuffers.clear();
        m_IndexBuffers.clear();

        int submeshCount = m_mesh->GetSubMeshCount();
        for (int i = 0; i < submeshCount; ++i)
        {
            std::shared_ptr<VertexBuffer> vertexBuffer = commandList->CopyVertexBuffer(m_mesh->GetVertices()[i]);
            const std::wstring meshStem = m_mesh->GetSourcePath().stem().wstring();
            vertexBuffer->SetName(L"DMesh " + meshStem + L" Sub" + std::to_wstring(i) + L" VB");
            m_VertexBuffers.push_back(vertexBuffer);
            std::shared_ptr<IndexBuffer> indexBuffer = commandList->CopyIndexBuffer(m_mesh->GetIndices()[i]);
            indexBuffer->SetName(L"DMesh " + meshStem + L" Sub" + std::to_wstring(i) + L" IB");
            m_IndexBuffers.push_back(indexBuffer);
        }

        if (m_pipelineStateObjects.empty())
            Initialize(renderContext);

        m_meshDirty = false;
    }

    struct ObjectData
    {
        DirectX::XMMATRIX worldMatrix;
        DirectX::XMFLOAT4 color;
        uint32_t useInstanceMatrix;
    } obj;

    obj.worldMatrix = XMMatrixTranspose(m_worldMatrix);
    obj.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    obj.useInstanceMatrix = 0;

    commandList->SetGraphicsDynamicConstantBuffer(static_cast<UINT>(RootParameterType::ObjectCB), obj);

    const uint32_t slotCount = static_cast<uint32_t>(MaterialTextureSlot::Count);
    const D3D12_CPU_DESCRIPTOR_HANDLE whiteSRV = DefaultTextures::GetWhiteSRV();

    if (m_VertexBuffers.size() != m_pipelineStateObjects.size()
        || m_IndexBuffers.size() != m_pipelineStateObjects.size())
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "MeshRenderProxy::GatherDrawCalls: submesh count drift (PSO={}, VB={}, IB={}); clamping to minimum",
            m_pipelineStateObjects.size(), m_VertexBuffers.size(), m_IndexBuffers.size());
    }
    const size_t drawCount = (std::min)({ m_pipelineStateObjects.size(), m_VertexBuffers.size(), m_IndexBuffers.size() });

    for (size_t i = 0; i < drawCount; ++i)
    {
        if (!m_pipelineStateObjects[i])
            continue;
        commandList->SetPipelineState(m_pipelineStateObjects[i]);
        commandList->SetPrimitiveTopology(m_PrimitiveTopology);

        DMaterial* material = m_mesh ? m_mesh->GetMaterial(static_cast<int>(i)) : nullptr;
        if (material)
        {
            MaterialCB materialCB {};
            material->FillMaterialCB(materialCB);
            commandList->SetGraphicsDynamicConstantBuffer(static_cast<UINT>(RootParameterType::MaterialCB),
                sizeof(MaterialCB), &materialCB);
        }

        auto submeshIt = m_textures.find(static_cast<uint32_t>(i));
        for (uint32_t slot = 0; slot < slotCount; ++slot)
        {
            std::shared_ptr<DirectX12Texture> tex;
            if (submeshIt != m_textures.end())
            {
                auto slotIt = submeshIt->second.find(slot);
                if (slotIt != submeshIt->second.end())
                    tex = slotIt->second;
            }

            if (tex)
            {
                commandList->SetShaderResourceView(static_cast<uint32_t>(RootParameterType::Texture), slot, tex,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }
            else
            {
                commandList->SetShaderResourceView(static_cast<uint32_t>(RootParameterType::Texture), slot, whiteSRV);
            }
        }

        commandList->SetVertexBuffer(0, m_VertexBuffers[i]);

        auto indexCount = m_IndexBuffers[i]->GetNumIndices();
        auto vertexCount = m_VertexBuffers[i]->GetNumVertices();

        if (indexCount > 0)
        {
            commandList->SetIndexBuffer(m_IndexBuffers[i]);
            commandList->DrawIndexed(indexCount, 1u, 0u, 0u, 0u);
        }
        else if (vertexCount > 0)
        {
            commandList->Draw(vertexCount, 1u, 0u, 0u);
        }
    }
}

bool MeshRenderProxy::SubmeshContributesToShadowMap(const DMaterial* material)
{
    return !(material && HasAny(material->GetFlags(), MaterialFlags::AlphaBlend));
}

void MeshRenderProxy::GatherShadowDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext, const ShadowView& view)
{
    if (!m_mesh)
        return;

    if (!DELTA_ENSURE(renderContext && renderContext->renderManager && renderContext->commandList))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "MeshRenderProxy::GatherShadowDrawCalls skipped: renderContext/renderManager/commandList is null");
        return;
    }

    const ShadowDepthPSO* shadowPso = renderContext->renderManager->GetShadowDepthPSO();
    if (!DELTA_ENSURE(shadowPso))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "MeshRenderProxy::GatherShadowDrawCalls skipped: shadow depth PSO is null");
        return;
    }

    std::shared_ptr<CommandList> commandList = renderContext->commandList;

    if (m_meshDirty)
    {
        m_VertexBuffers.clear();
        m_IndexBuffers.clear();

        const int submeshCount = m_mesh->GetSubMeshCount();
        for (int i = 0; i < submeshCount; ++i)
        {
            std::shared_ptr<VertexBuffer> vertexBuffer = commandList->CopyVertexBuffer(m_mesh->GetVertices()[i]);
            const std::wstring meshStem = m_mesh->GetSourcePath().stem().wstring();
            vertexBuffer->SetName(L"DMesh " + meshStem + L" Sub" + std::to_wstring(i) + L" VB Shadow");
            m_VertexBuffers.push_back(vertexBuffer);
            std::shared_ptr<IndexBuffer> indexBuffer = commandList->CopyIndexBuffer(m_mesh->GetIndices()[i]);
            indexBuffer->SetName(L"DMesh " + meshStem + L" Sub" + std::to_wstring(i) + L" IB Shadow");
            m_IndexBuffers.push_back(indexBuffer);
        }

        if (m_pipelineStateObjects.empty())
            Initialize(renderContext);

        m_meshDirty = false;
    }

    const XMMATRIX viewProjCb = view.viewProj;
    const XMMATRIX worldCb = XMMatrixTranspose(m_worldMatrix);

    commandList->SetGraphicsRootSignature(shadowPso->GetRootSignature());
    commandList->SetPipelineState(shadowPso->GetPSO2D());
    commandList->SetPrimitiveTopology(m_PrimitiveTopology);

    const int submeshCount = m_mesh->GetSubMeshCount();
    for (int i = 0; i < submeshCount; ++i)
    {
        DMaterial* material = m_mesh->GetMaterial(i);
        if (!SubmeshContributesToShadowMap(material))
            continue;

        if (i >= static_cast<int>(m_VertexBuffers.size()) || i >= static_cast<int>(m_IndexBuffers.size()))
            continue;

        commandList->SetGraphicsDynamicConstantBuffer(ShadowDepthRS::ViewProjCB, viewProjCb);
        commandList->SetGraphicsDynamicConstantBuffer(ShadowDepthRS::WorldMatrixCB, worldCb);

        commandList->SetVertexBuffer(0, m_VertexBuffers[static_cast<size_t>(i)]);

        const auto indexCount = m_IndexBuffers[static_cast<size_t>(i)]->GetNumIndices();
        const auto vertexCount = m_VertexBuffers[static_cast<size_t>(i)]->GetNumVertices();

        if (indexCount > 0)
        {
            commandList->SetIndexBuffer(m_IndexBuffers[static_cast<size_t>(i)]);
            commandList->DrawIndexed(static_cast<uint32_t>(indexCount), 1u, 0u, 0u, 0u);
        }
        else if (vertexCount > 0)
        {
            commandList->Draw(static_cast<uint32_t>(vertexCount), 1u, 0u, 0u);
        }
    }
}

size_t DeltaEngine::MeshRenderProxy::GetIndexCount() const
{
    size_t indexCount = 0;
    if (!m_IndexBuffers.empty())
        indexCount = m_IndexBuffers[0]->GetNumIndices();

    return indexCount;
}

size_t DeltaEngine::MeshRenderProxy::GetVertexCount() const
{
    size_t vertexCount = 0;
    if (!m_VertexBuffers.empty())
        vertexCount = m_VertexBuffers[0]->GetNumVertices();

    return vertexCount;
}
