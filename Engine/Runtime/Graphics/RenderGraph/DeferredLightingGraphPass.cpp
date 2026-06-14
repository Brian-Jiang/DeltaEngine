#include "Runtime/Graphics/RenderGraph/DeferredLightingGraphPass.h"

#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/PipelineStateObject.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"
#include "Runtime/Graphics/Structures/DeferredLightingRootParameterType.h"

DELTA_ENGINE_NS_BEGIN

DeferredLightingGraphPass::DeferredLightingGraphPass(RenderGraphTextureHandle albedoInput,
    RenderGraphTextureHandle normalInput,
    RenderGraphTextureHandle materialInput,
    RenderGraphTextureHandle emissiveInput,
    RenderGraphTextureHandle depthInput,
    RenderGraphTextureHandle sceneColorOutput,
    D3D12_CPU_DESCRIPTOR_HANDLE albedoSrv,
    D3D12_CPU_DESCRIPTOR_HANDLE normalSrv,
    D3D12_CPU_DESCRIPTOR_HANDLE materialSrv,
    D3D12_CPU_DESCRIPTOR_HANDLE emissiveSrv,
    D3D12_CPU_DESCRIPTOR_HANDLE depthSrv,
    D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRtv,
    CD3DX12_VIEWPORT viewport,
    D3D12_RECT scissorRect,
    DXRenderManager* renderManager,
    std::shared_ptr<DXGraphicsContext> graphicsContext)
    : m_albedoInput(albedoInput)
    , m_normalInput(normalInput)
    , m_materialInput(materialInput)
    , m_emissiveInput(emissiveInput)
    , m_depthInput(depthInput)
    , m_sceneColorOutput(sceneColorOutput)
    , m_albedoSrv(albedoSrv)
    , m_normalSrv(normalSrv)
    , m_materialSrv(materialSrv)
    , m_emissiveSrv(emissiveSrv)
    , m_depthSrv(depthSrv)
    , m_sceneColorRtv(sceneColorRtv)
    , m_viewport(viewport)
    , m_scissorRect(scissorRect)
    , m_renderManager(renderManager)
    , m_graphicsContext(std::move(graphicsContext))
{
}

const char* DeferredLightingGraphPass::GetName() const
{
    return "DeferredLighting";
}

void DeferredLightingGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Read(m_albedoInput, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Read(m_normalInput, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Read(m_materialInput, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Read(m_emissiveInput, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Read(m_depthInput, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Write(m_sceneColorOutput, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void DeferredLightingGraphPass::Execute(const RenderGraphContext& context) const
{
    if (!context.commandList || !m_renderManager || !m_graphicsContext)
        return;

    if (!m_renderManager->EnsureDeferredLightingPipeline())
        return;

    auto rootSignature = m_renderManager->GetDeferredLightingRootSignature();
    auto pso = m_renderManager->GetDeferredLightingPSO();
    if (!rootSignature || !pso)
        return;

    CommandList& commandList = *context.commandList;
    commandList.SetViewport(m_viewport);
    commandList.SetScissorRect(m_scissorRect);
    commandList.GetD3D12CommandList()->OMSetRenderTargets(1, &m_sceneColorRtv, FALSE, nullptr);
    commandList.SetGraphicsRootSignature(rootSignature);
    commandList.SetPipelineState(pso);

    const uint32_t gbufferTableSlot = static_cast<uint32_t>(DeferredLightingRootParameterType::GBufferTextures);
    commandList.SetShaderResourceView(gbufferTableSlot, 0u, m_albedoSrv);
    commandList.SetShaderResourceView(gbufferTableSlot, 1u, m_normalSrv);
    commandList.SetShaderResourceView(gbufferTableSlot, 2u, m_materialSrv);
    commandList.SetShaderResourceView(gbufferTableSlot, 3u, m_emissiveSrv);
    commandList.SetShaderResourceView(gbufferTableSlot, 4u, m_depthSrv);

    if (m_graphicsContext->activeRenderCamera.has_value())
    {
        commandList.SetGraphicsDynamicConstantBuffer(
            static_cast<UINT>(DeferredLightingRootParameterType::CameraCB),
            m_graphicsContext->activeRenderCamera->cb);
    }

    m_graphicsContext->ApplyLightBuffersToCommandList(
        static_cast<uint32_t>(DeferredLightingRootParameterType::LightCB),
        static_cast<uint32_t>(DeferredLightingRootParameterType::DirectionalLights),
        static_cast<uint32_t>(DeferredLightingRootParameterType::PointLights),
        static_cast<uint32_t>(DeferredLightingRootParameterType::SpotLights));

    m_renderManager->StageIBLDescriptors(commandList,
        static_cast<int32_t>(DeferredLightingRootParameterType::IBLTextures));
    m_renderManager->StageShadowDescriptors(commandList,
        static_cast<int32_t>(DeferredLightingRootParameterType::ShadowMaps),
        static_cast<int32_t>(DeferredLightingRootParameterType::ShadowCB));

    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.Draw(3u, 1u, 0u, 0u);
}

DELTA_ENGINE_NS_END
