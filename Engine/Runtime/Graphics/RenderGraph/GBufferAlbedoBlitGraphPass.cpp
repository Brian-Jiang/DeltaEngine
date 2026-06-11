#include "Runtime/Graphics/RenderGraph/GBufferAlbedoBlitGraphPass.h"

#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/PipelineStateObject.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"

DELTA_ENGINE_NS_BEGIN

GBufferAlbedoBlitGraphPass::GBufferAlbedoBlitGraphPass(RenderGraphTextureHandle albedoInput,
    RenderGraphTextureHandle sceneColorOutput,
    D3D12_CPU_DESCRIPTOR_HANDLE albedoSrv,
    D3D12_CPU_DESCRIPTOR_HANDLE sceneColorRtv,
    CD3DX12_VIEWPORT viewport,
    D3D12_RECT scissorRect,
    DXRenderManager* renderManager)
    : m_albedoInput(albedoInput)
    , m_sceneColorOutput(sceneColorOutput)
    , m_albedoSrv(albedoSrv)
    , m_sceneColorRtv(sceneColorRtv)
    , m_viewport(viewport)
    , m_scissorRect(scissorRect)
    , m_renderManager(renderManager)
{
}

const char* GBufferAlbedoBlitGraphPass::GetName() const
{
    return "GBufferAlbedoBlit";
}

void GBufferAlbedoBlitGraphPass::Setup(RenderGraphBuilder& builder)
{
    builder.Read(m_albedoInput, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    builder.Write(m_sceneColorOutput, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void GBufferAlbedoBlitGraphPass::Execute(const RenderGraphContext& context) const
{
    if (!context.commandList || !m_renderManager)
        return;

    if (!m_renderManager->EnsureGBufferAlbedoBlitPipeline())
        return;

    auto rootSignature = m_renderManager->GetGBufferAlbedoBlitRootSignature();
    auto pso = m_renderManager->GetGBufferAlbedoBlitPSO();
    if (!rootSignature || !pso)
        return;

    CommandList& commandList = *context.commandList;
    commandList.SetViewport(m_viewport);
    commandList.SetScissorRect(m_scissorRect);
    commandList.GetD3D12CommandList()->OMSetRenderTargets(1, &m_sceneColorRtv, FALSE, nullptr);
    commandList.SetGraphicsRootSignature(rootSignature);
    commandList.SetPipelineState(pso);
    commandList.SetShaderResourceView(0u, 0u, m_albedoSrv);
    commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.Draw(3u, 1u, 0u, 0u);
}

DELTA_ENGINE_NS_END
