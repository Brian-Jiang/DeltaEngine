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
#include "Graphics/DirectX/RenderTarget.h"
#include "Graphics/DirectX/VertexBuffer.h"
#include "Graphics/MaterialConstants.h"
#include "Graphics/Structures/RootParameterType.h"
#include "Graphics/Structures/GBufferRootParameterType.h"
#include "Graphics/Structures/Vertex.h"
#include "Graphics/Shadow/ShadowDepthPSO.h"
#include "Graphics/Shadow/ShadowView.h"
#include "Core/DMaterial.h"
#include "Core/DMesh.h"
#include "Core/DShader.h"
#include "Core/DTexture.h"
#include "Runtime/Utils/StringUtils.h"
#include "Runtime/Logging/LogChannels.h"

#include <format>

using namespace DeltaEngine;
using namespace DirectX;

MeshRenderProxy::MeshRenderProxy()
    : m_mesh(nullptr)
    , m_PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
    , m_worldMatrix(DirectX::XMMatrixIdentity())
    , m_meshDirty(true)
{
}

DeltaEngine::MeshRenderProxy::MeshRenderProxy(DMesh* mesh, std::shared_ptr<MeshRendererSettings> settings,
    const std::vector<DMaterial*>& materialOverrides)
    : m_mesh(mesh)
    , m_materialOverrides(materialOverrides)
    , m_settings(settings)
    , m_PrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) // todo : get primitive topology from mesh
    , m_worldMatrix(DirectX::XMMatrixIdentity())
    , m_meshDirty(true)
{
}

DeltaEngine::MeshRenderProxy::~MeshRenderProxy()
{
}

bool MeshRenderProxy::HasExclusiveGPUResources() const
{
    return !m_VertexBuffers.empty() || !m_IndexBuffers.empty() || !m_pipelineStateObjects.empty()
        || !m_gbufferPipelineStateObjects.empty();
}

void MeshRenderProxy::ReleaseSharedReferences()
{
    m_textures.clear();
}

DMaterial* MeshRenderProxy::ResolveSubmeshMaterial(const DMesh* mesh, int submeshIndex,
    const std::vector<DMaterial*>& materialOverrides)
{
    if (!mesh || submeshIndex < 0 || submeshIndex >= mesh->GetSubMeshCount())
        return nullptr;

    if (materialOverrides.empty())
        return mesh->GetMaterial(submeshIndex);

    const size_t index = static_cast<size_t>(submeshIndex);
    if (index < materialOverrides.size() && materialOverrides[index])
        return materialOverrides[index];

    return mesh->GetMaterial(submeshIndex);
}

DMaterial* MeshRenderProxy::GetEffectiveMaterial(int submeshIndex) const
{
    return ResolveSubmeshMaterial(m_mesh, submeshIndex, m_materialOverrides);
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
    m_gbufferPipelineStateObjects.clear();
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
    DXGI_SAMPLE_DESC sampleDesc = renderContext->renderManager->GetRenderTarget()->GetSampleDesc();

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = backBufferFormat;

    m_textures.clear();
    m_pipelineStateObjects.clear();
    m_gbufferPipelineStateObjects.clear();
    m_builtShaderGenerations.clear();
    m_builtMaterialPsoKeys.clear();
    const std::string meshName = m_mesh->GetSourcePath().stem().string();

    ISlangBlob* gbufferVertexShader = renderContext->renderManager->GetGBufferVertexShaderBlob();
    ISlangBlob* gbufferPixelShader = renderContext->renderManager->GetGBufferPixelShaderBlob();
    const auto gbufferRootSignature = renderContext->renderManager->GetGBufferRootSignature();
    const D3D12_RT_FORMAT_ARRAY gbufferRtvFormats = renderContext->renderManager->GetGBufferRTVFormats();
    const DXGI_SAMPLE_DESC gbufferSampleDesc { 1, 0 };

    for (int i = 0; i < m_mesh->GetSubMeshCount(); ++i)
    {
        auto material = GetEffectiveMaterial(i);
        // Record the shader generation for this submesh in every branch (skip or build) to stay index-aligned.
        DShader* submeshShader = material ? material->GetShader() : nullptr;
        m_builtShaderGenerations.push_back(submeshShader ? submeshShader->GetCompileGeneration() : 0u);
        m_builtMaterialPsoKeys.push_back(ComputeMaterialPsoKey(material));
        if (!DELTA_ENSURE(material))
        {
            DLOG(LogRenderer, ELogLevel::Warning,
                "MeshRenderProxy::Initialize: submesh {} of '{}' has null material; skipping submesh",
                i, meshName);
            m_pipelineStateObjects.push_back(nullptr);
            m_gbufferPipelineStateObjects.push_back(nullptr);
            continue;
        }
        if (!DELTA_ENSURE(material->GetShader()))
        {
            DLOG(LogRenderer, ELogLevel::Warning,
                "MeshRenderProxy::Initialize: submesh {} of '{}' has material with null shader; skipping submesh",
                i, meshName);
            m_pipelineStateObjects.push_back(nullptr);
            m_gbufferPipelineStateObjects.push_back(nullptr);
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
            m_gbufferPipelineStateObjects.push_back(nullptr);
            continue;
        }

        std::vector<D3D12_INPUT_ELEMENT_DESC> layout = material->GetShader()->GetInputLayout();
        if (!DELTA_ENSURE(!layout.empty()))
        {
            DLOG(LogRenderer, ELogLevel::Warning,
                "MeshRenderProxy::Initialize: submesh {} of '{}' has empty shader input layout; skipping submesh",
                i, meshName);
            m_pipelineStateObjects.push_back(nullptr);
            m_gbufferPipelineStateObjects.push_back(nullptr);
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
            const std::wstring psoName = StringUtils::Utf8ToWString(
                std::format("PSO MeshRenderProxy Sub{}", i));
            m_pipelineStateObjects.back()->GetD3D12PipelineState()->SetName(psoName.c_str());
        }

        if (gbufferRootSignature && gbufferVertexShader && gbufferPixelShader)
        {
            CD3DX12_SHADER_BYTECODE gbufferVsBytecode {
                const_cast<void*>(gbufferVertexShader->getBufferPointer()), gbufferVertexShader->getBufferSize() };
            CD3DX12_SHADER_BYTECODE gbufferPsBytecode {
                const_cast<void*>(gbufferPixelShader->getBufferPointer()), gbufferPixelShader->getBufferSize() };

            pipelineStateStream.pRootSignature = gbufferRootSignature->GetD3D12RootSignature().Get();
            pipelineStateStream.VS = gbufferVsBytecode;
            pipelineStateStream.PS = gbufferPsBytecode;
            pipelineStateStream.RTVFormats = gbufferRtvFormats;
            pipelineStateStream.SampleDesc = gbufferSampleDesc;

            m_gbufferPipelineStateObjects.push_back(device->CreatePipelineStateObject(pipelineStateStream));
            if (m_gbufferPipelineStateObjects.back())
            {
                const std::wstring psoName = StringUtils::Utf8ToWString(
                    std::format("PSO MeshRenderProxy GBuffer Sub{}", i));
                m_gbufferPipelineStateObjects.back()->GetD3D12PipelineState()->SetName(psoName.c_str());
            }
        }
        else
        {
            m_gbufferPipelineStateObjects.push_back(nullptr);
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

void MeshRenderProxy::EnsureDrawResourcesReady(std::shared_ptr<DXGraphicsContext> renderContext)
{
    if (!m_mesh || !DELTA_ENSURE(renderContext && renderContext->commandList))
        return;

    std::shared_ptr<CommandList> commandList = renderContext->commandList;

    if (m_meshDirty)
    {
        m_VertexBuffers.clear();
        m_IndexBuffers.clear();

        const int submeshCount = m_mesh->GetSubMeshCount();
        for (int i = 0; i < submeshCount; ++i)
        {
            std::shared_ptr<VertexBuffer> vertexBuffer = commandList->CopyVertexBuffer(m_mesh->GetVertices()[i]);
            const std::string meshStem = StringUtils::PathToUtf8(m_mesh->GetSourcePath().stem());
            vertexBuffer->SetName(std::format("DMesh {} Sub{} VB", meshStem, i));
            m_VertexBuffers.push_back(vertexBuffer);
            std::shared_ptr<IndexBuffer> indexBuffer = commandList->CopyIndexBuffer(m_mesh->GetIndices()[i]);
            indexBuffer->SetName(std::format("DMesh {} Sub{} IB", meshStem, i));
            m_IndexBuffers.push_back(indexBuffer);
        }

        if (m_pipelineStateObjects.empty() && m_gbufferPipelineStateObjects.empty())
            Initialize(renderContext);

        m_meshDirty = false;
    }
    else if (ShadersChanged() || MaterialPipelineStateChanged())
    {
        Initialize(renderContext);
    }
}

void MeshRenderProxy::BindObjectConstantBuffer(std::shared_ptr<DXGraphicsContext> renderContext) const
{
    struct ObjectData
    {
        DirectX::XMMATRIX worldMatrix;
        DirectX::XMFLOAT4 color;
        uint32_t useInstanceMatrix;
    } obj;

    obj.worldMatrix = XMMatrixTranspose(m_worldMatrix);
    obj.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    obj.useInstanceMatrix = 0;

    const bool gbufferPass = renderContext->activePass == ScenePassType::GBuffer;
    const uint32_t objectCbSlot = gbufferPass
        ? static_cast<uint32_t>(GBufferRootParameterType::ObjectCB)
        : static_cast<uint32_t>(RootParameterType::ObjectCB);

    renderContext->commandList->SetGraphicsDynamicConstantBuffer(objectCbSlot, obj);
}

void MeshRenderProxy::DrawSubmeshInternal(std::shared_ptr<DXGraphicsContext> renderContext, size_t submeshIndex)
{
    if (!m_mesh || !renderContext || !renderContext->commandList)
        return;

    const bool gbufferPass = renderContext->activePass == ScenePassType::GBuffer;
    const auto& activePsos = gbufferPass ? m_gbufferPipelineStateObjects : m_pipelineStateObjects;
    const uint32_t materialCbSlot = gbufferPass
        ? static_cast<uint32_t>(GBufferRootParameterType::MaterialCB)
        : static_cast<uint32_t>(RootParameterType::MaterialCB);
    const uint32_t textureSlot = gbufferPass
        ? static_cast<uint32_t>(GBufferRootParameterType::Texture)
        : static_cast<uint32_t>(RootParameterType::Texture);

    if (submeshIndex >= activePsos.size() || submeshIndex >= m_VertexBuffers.size()
        || submeshIndex >= m_IndexBuffers.size())
    {
        return;
    }

    if (!activePsos[submeshIndex])
        return;

    std::shared_ptr<CommandList> commandList = renderContext->commandList;
    DMaterial* material = GetEffectiveMaterial(static_cast<int>(submeshIndex));

    commandList->SetPipelineState(activePsos[submeshIndex]);
    commandList->SetPrimitiveTopology(m_PrimitiveTopology);

    if (material)
    {
        MaterialCB materialCB {};
        material->FillMaterialCB(materialCB);
        commandList->SetGraphicsDynamicConstantBuffer(materialCbSlot, sizeof(MaterialCB), &materialCB);
    }

    const uint32_t slotCount = static_cast<uint32_t>(MaterialTextureSlot::Count);
    const D3D12_CPU_DESCRIPTOR_HANDLE whiteSRV = DefaultTextures::GetWhiteSRV();
    auto submeshIt = m_textures.find(static_cast<uint32_t>(submeshIndex));
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
            commandList->SetShaderResourceView(textureSlot, slot, tex,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
        else
        {
            commandList->SetShaderResourceView(textureSlot, slot, whiteSRV);
        }
    }

    commandList->SetVertexBuffer(0, m_VertexBuffers[submeshIndex]);

    const auto indexCount = m_IndexBuffers[submeshIndex]->GetNumIndices();
    const auto vertexCount = m_VertexBuffers[submeshIndex]->GetNumVertices();

    if (indexCount > 0)
    {
        commandList->SetIndexBuffer(m_IndexBuffers[submeshIndex]);
        commandList->DrawIndexed(indexCount, 1u, 0u, 0u, 0u);
    }
    else if (vertexCount > 0)
    {
        commandList->Draw(vertexCount, 1u, 0u, 0u);
    }
}

void MeshRenderProxy::DrawSubmesh(std::shared_ptr<DXGraphicsContext> renderContext, size_t submeshIndex)
{
    if (!m_mesh)
        return;

    if (!DELTA_ENSURE(renderContext && renderContext->commandList))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "MeshRenderProxy::DrawSubmesh skipped: renderContext or commandList is null");
        return;
    }

    EnsureDrawResourcesReady(renderContext);
    BindObjectConstantBuffer(renderContext);
    DrawSubmeshInternal(renderContext, submeshIndex);
}

float MeshRenderProxy::ComputeSubmeshSortDepth(const DMesh* mesh, int submeshIndex, XMMATRIX worldMatrix,
    XMVECTOR cameraPosition)
{
    XMFLOAT3 center { 0.0f, 0.0f, 0.0f };
    if (mesh && submeshIndex >= 0 && submeshIndex < mesh->GetSubMeshCount())
        center = mesh->GetSubMeshLocalCenter(submeshIndex);

    const XMVECTOR centerWorld = XMVector3TransformCoord(XMLoadFloat3(&center), worldMatrix);
    const XMVECTOR delta = XMVectorSubtract(centerWorld, cameraPosition);
    return XMVectorGetX(XMVector3Length(delta));
}

void MeshRenderProxy::AppendTransparentDrawEntries(std::vector<TransparentDrawEntry>& out,
    XMVECTOR cameraPosition) const
{
    if (!m_mesh)
        return;

    const int submeshCount = m_mesh->GetSubMeshCount();
    for (int i = 0; i < submeshCount; ++i)
    {
        DMaterial* material = GetEffectiveMaterial(i);
        if (!SubmeshContributesToTransparentPass(material))
            continue;

        TransparentDrawEntry entry {};
        entry.proxy = const_cast<MeshRenderProxy*>(this);
        entry.submeshIndex = static_cast<size_t>(i);
        entry.sortDepth = ComputeSubmeshSortDepth(m_mesh, i, m_worldMatrix, cameraPosition);
        out.push_back(entry);
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

    EnsureDrawResourcesReady(renderContext);
    BindObjectConstantBuffer(renderContext);

    const bool gbufferPass = renderContext->activePass == ScenePassType::GBuffer;
    const auto& activePsos = gbufferPass ? m_gbufferPipelineStateObjects : m_pipelineStateObjects;

    if (activePsos.size() != m_VertexBuffers.size() || m_IndexBuffers.size() != activePsos.size())
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "MeshRenderProxy::GatherDrawCalls: submesh count drift (PSO={}, VB={}, IB={}); clamping to minimum",
            activePsos.size(), m_VertexBuffers.size(), m_IndexBuffers.size());
    }
    const size_t drawCount = (std::min)({ activePsos.size(), m_VertexBuffers.size(), m_IndexBuffers.size() });

    for (size_t i = 0; i < drawCount; ++i)
    {
        DMaterial* material = m_mesh ? GetEffectiveMaterial(static_cast<int>(i)) : nullptr;
        const ScenePassType pass = renderContext->activePass;
        if (pass == ScenePassType::Forward || pass == ScenePassType::GBuffer)
        {
            if (!SubmeshContributesToOpaquePass(material))
                continue;
        }
        else if (pass == ScenePassType::Transparent)
        {
            if (!SubmeshContributesToTransparentPass(material))
                continue;
        }

        DrawSubmeshInternal(renderContext, i);
    }
}

bool MeshRenderProxy::ShadersChanged() const
{
    if (!m_mesh || m_builtShaderGenerations.empty())
        return false;

    const int submeshCount = m_mesh->GetSubMeshCount();
    if (static_cast<size_t>(submeshCount) != m_builtShaderGenerations.size())
        return true;

    for (int i = 0; i < submeshCount; ++i)
    {
        DMaterial* material = GetEffectiveMaterial(i);
        DShader* shader = material ? material->GetShader() : nullptr;
        const uint32_t gen = shader ? shader->GetCompileGeneration() : 0u;
        if (gen != m_builtShaderGenerations[static_cast<size_t>(i)])
            return true;
    }

    return false;
}

uint32_t MeshRenderProxy::ComputeMaterialPsoKey(const DMaterial* material)
{
    if (!material)
        return 0u;
    // Only flags that are baked into the graphics PSO (blend + rasterizer cull).
    const MaterialFlags psoMask = MaterialFlags::AlphaBlend | MaterialFlags::DoubleSided;
    return static_cast<uint32_t>(material->GetFlags() & psoMask);
}

bool MeshRenderProxy::MaterialPipelineStateChanged() const
{
    if (!m_mesh || m_builtMaterialPsoKeys.empty())
        return false;

    const int submeshCount = m_mesh->GetSubMeshCount();
    if (static_cast<size_t>(submeshCount) != m_builtMaterialPsoKeys.size())
        return true;

    for (int i = 0; i < submeshCount; ++i)
    {
        if (ComputeMaterialPsoKey(GetEffectiveMaterial(i)) != m_builtMaterialPsoKeys[static_cast<size_t>(i)])
            return true;
    }

    return false;
}

bool MeshRenderProxy::SubmeshContributesToOpaquePass(const DMaterial* material)
{
    return !(material && HasAny(material->GetFlags(), MaterialFlags::AlphaBlend));
}

bool MeshRenderProxy::SubmeshContributesToTransparentPass(const DMaterial* material)
{
    return material && HasAny(material->GetFlags(), MaterialFlags::AlphaBlend);
}

bool MeshRenderProxy::SubmeshContributesToShadowMap(const DMaterial* material)
{
    return SubmeshContributesToOpaquePass(material);
}

bool MeshRenderProxy::SubmeshContributesToGBuffer(const DMaterial* material)
{
    return SubmeshContributesToOpaquePass(material);
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
            const std::string meshStem = StringUtils::PathToUtf8(m_mesh->GetSourcePath().stem());
            vertexBuffer->SetName(std::format("DMesh {} Sub{} VB Shadow", meshStem, i));
            m_VertexBuffers.push_back(vertexBuffer);
            std::shared_ptr<IndexBuffer> indexBuffer = commandList->CopyIndexBuffer(m_mesh->GetIndices()[i]);
            indexBuffer->SetName(std::format("DMesh {} Sub{} IB Shadow", meshStem, i));
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
        DMaterial* material = GetEffectiveMaterial(i);
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
