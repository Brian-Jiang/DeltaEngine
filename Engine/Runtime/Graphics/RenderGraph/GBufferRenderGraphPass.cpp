#include "Runtime/Graphics/RenderGraph/GBufferRenderGraphPass.h"

#include <utility>

#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphContext.h"

DELTA_ENGINE_NS_BEGIN

GBufferRenderGraphPass::GBufferRenderGraphPass(RenderGraphTextureHandle albedo,
    RenderGraphTextureHandle normal,
    RenderGraphTextureHandle material,
    RenderGraphTextureHandle emissive,
    RenderGraphTextureHandle depth,
    D3D12_CPU_DESCRIPTOR_HANDLE albedoRtv,
    D3D12_CPU_DESCRIPTOR_HANDLE normalRtv,
    D3D12_CPU_DESCRIPTOR_HANDLE materialRtv,
    D3D12_CPU_DESCRIPTOR_HANDLE emissiveRtv,
    D3D12_CPU_DESCRIPTOR_HANDLE depthDsv,
    std::shared_ptr<RootSignature> rootSignature,
    CD3DX12_VIEWPORT viewport,
    D3D12_RECT scissorRect,
    std::shared_ptr<DXGraphicsContext> graphicsContext,
    SceneDrawCallback drawCallback,
    RenderGraphTextureHandle velocity,
    D3D12_CPU_DESCRIPTOR_HANDLE velocityRtv)
    : m_albedo(albedo)
    , m_normal(normal)
    , m_material(material)
    , m_emissive(emissive)
    , m_depth(depth)
    , m_velocity(velocity)
    , m_albedoRtv(albedoRtv)
    , m_normalRtv(normalRtv)
    , m_materialRtv(materialRtv)
    , m_emissiveRtv(emissiveRtv)
    , m_velocityRtv(velocityRtv)
    , m_depthDsv(depthDsv)
    , m_rootSignature(std::move(rootSignature))
    , m_viewport(viewport)
    , m_scissorRect(scissorRect)
    , m_graphicsContext(std::move(graphicsContext))
    , m_drawCallback(std::move(drawCallback))
{
}

const char* GBufferRenderGraphPass::GetName() const
{
    return "GBuffer";
}

void GBufferRenderGraphPass::Setup(RenderGraphBuilder& builder)
{
    const RenderGraphClearValue blackClear = RenderGraphClearValue::Color4(0.0f, 0.0f, 0.0f, 0.0f);
    builder.Write(m_albedo, D3D12_RESOURCE_STATE_RENDER_TARGET, blackClear);
    builder.Write(m_normal, D3D12_RESOURCE_STATE_RENDER_TARGET, blackClear);
    builder.Write(m_material, D3D12_RESOURCE_STATE_RENDER_TARGET, blackClear);
    builder.Write(m_emissive, D3D12_RESOURCE_STATE_RENDER_TARGET, blackClear);
    builder.Write(m_depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, RenderGraphClearValue::DepthStencil(1.0f));
    if (m_velocity.IsValid())
        builder.Write(m_velocity, D3D12_RESOURCE_STATE_RENDER_TARGET, blackClear);
}

void GBufferRenderGraphPass::Execute(const RenderGraphContext& context) const
{
    if (!context.commandList || !m_graphicsContext)
        return;

    CommandList& commandList = *context.commandList;
    commandList.SetViewport(m_viewport);
    commandList.SetScissorRect(m_scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[5] = {
        m_albedoRtv, m_normalRtv, m_materialRtv, m_emissiveRtv, m_velocityRtv
    };
    const UINT rtvCount = m_velocityRtv.ptr != 0 ? 5u : 4u;
    commandList.GetD3D12CommandList()->OMSetRenderTargets(rtvCount, rtvs, FALSE, &m_depthDsv);

    if (m_rootSignature)
        commandList.SetGraphicsRootSignature(m_rootSignature);

    const ScenePassType previousPass = m_graphicsContext->activePass;
    m_graphicsContext->activePass = ScenePassType::GBuffer;

    if (m_drawCallback)
        m_drawCallback(m_graphicsContext);

    m_graphicsContext->activePass = previousPass;
}

DELTA_ENGINE_NS_END
