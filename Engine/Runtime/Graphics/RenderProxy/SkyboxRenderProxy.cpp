#include "Runtime/Graphics/RenderProxy/SkyboxRenderProxy.h"

#include <d3dx12.h>
#include <dxcapi.h>

#include "Graphics/DXGraphicsContext.h"
#include "Graphics/DXRenderManager.h"
#include "Graphics/DirectX/CommandList.h"
#include "Graphics/DirectX/Device.h"
#include "Graphics/DirectX/DirectX12Texture.h"
#include "Graphics/DirectX/PipelineStateObject.h"
#include "Graphics/DirectX/RootSignature.h"
#include "Graphics/Structures/RootParameterType.h"
#include "Core/DMaterial.h"
#include "Core/DShader.h"
#include "Core/DTexture.h"

using namespace DeltaEngine;

SkyboxRenderProxy::SkyboxRenderProxy(DTexture* cubemapTexture, DMaterial* material)
    : m_cubemapTexture(cubemapTexture)
    , m_material(material)
{
}

void SkyboxRenderProxy::Initialize(std::shared_ptr<DXGraphicsContext> renderContext)
{
    if (m_initialized)
        return;
    if (!DELTA_ENSURE(m_cubemapTexture))
    {
        DLOG(LogRenderer, ELogLevel::Warning, "SkyboxRenderProxy::Initialize skipped: cubemap texture is null");
        return;
    }
    if (!DELTA_ENSURE(m_material))
    {
        DLOG(LogRenderer, ELogLevel::Warning, "SkyboxRenderProxy::Initialize skipped: material is null");
        return;
    }
    if (!DELTA_ENSURE(m_material->GetShader()))
    {
        DLOG(LogRenderer, ELogLevel::Warning, "SkyboxRenderProxy::Initialize skipped: material shader is null");
        return;
    }
    if (!DELTA_ENSURE(renderContext && renderContext->commandList && renderContext->device && renderContext->renderManager))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "SkyboxRenderProxy::Initialize skipped: renderContext missing commandList/device/renderManager");
        return;
    }
    if (!DELTA_ENSURE(renderContext->renderManager->GetRootSignature()))
    {
        DLOG(LogRenderer, ELogLevel::Warning, "SkyboxRenderProxy::Initialize skipped: root signature is null");
        return;
    }

    // Upload cubemap; LoadTexture calls CreateCubemapSRV() because IsCubemap() is true.
    m_gpuCubemap = renderContext->commandList->LoadTexture(m_cubemapTexture);

    // Depth: LESS_EQUAL so depth=1.0 passes; ZERO write mask so skybox never occludes geometry.
    CD3DX12_DEPTH_STENCIL_DESC depthDesc(D3D12_DEFAULT);
    depthDesc.DepthFunc      = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    // CullMode NONE: camera is inside the cube, so back-face culling would hide everything.
    CD3DX12_RASTERIZER_DESC rasterDesc(D3D12_DEFAULT);
    rasterDesc.CullMode = D3D12_CULL_MODE_NONE;

    CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);

    ISlangBlob* vs = m_material->GetShader()->GetVertexShaderBlob();
    ISlangBlob* ps = m_material->GetShader()->GetPixelShaderBlob();
    if (!DELTA_ENSURE(vs && ps))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "SkyboxRenderProxy::Initialize skipped: shader missing VS or PS blob");
        return;
    }
    CD3DX12_SHADER_BYTECODE vsBytecode{ const_cast<void*>(vs->getBufferPointer()), vs->getBufferSize() };
    CD3DX12_SHADER_BYTECODE psBytecode{ const_cast<void*>(ps->getBufferPointer()), ps->getBufferSize() };

    DXGI_FORMAT backBufFmt  = DXGI_FORMAT_R16G16B16A16_FLOAT;
    DXGI_FORMAT depthFmt    = DXGI_FORMAT_D32_FLOAT;
    DXGI_SAMPLE_DESC sample = renderContext->device->GetMultisampleQualityLevels(backBufFmt);

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0]     = backBufFmt;

    // No InputLayout: omitted field zero-initializes to NumElements=0, valid for SV_VertexID shaders.
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE        pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_VS                    VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS                    PS;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER            RasterizerState;
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC            BlendState;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL         DepthStencilState;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY    PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT  DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC           SampleDesc;
    } stream;

    stream.pRootSignature      = renderContext->renderManager->GetRootSignature()->GetD3D12RootSignature().Get();
    stream.VS                  = vsBytecode;
    stream.PS                  = psBytecode;
    stream.RasterizerState     = rasterDesc;
    stream.BlendState          = blendDesc;
    stream.DepthStencilState   = depthDesc;
    stream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    stream.DSVFormat           = depthFmt;
    stream.RTVFormats          = rtvFormats;
    stream.SampleDesc          = sample;

    m_pso = renderContext->device->CreatePipelineStateObject(stream);
    if (!DELTA_ENSURE(m_pso))
    {
        DLOG(LogRenderer, ELogLevel::Warning, "SkyboxRenderProxy::Initialize: PSO creation failed");
        return;
    }
    m_pso->GetD3D12PipelineState()->SetName(L"PSO Skybox");

    m_initialized = true;
}

void SkyboxRenderProxy::GatherDrawCalls(std::shared_ptr<DXGraphicsContext> renderContext)
{
    if (!m_initialized)
        return;
    if (!DELTA_ENSURE(m_pso && m_gpuCubemap))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "SkyboxRenderProxy::GatherDrawCalls skipped: PSO or GPU cubemap is null after initialize");
        return;
    }
    if (!DELTA_ENSURE(renderContext && renderContext->commandList))
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "SkyboxRenderProxy::GatherDrawCalls skipped: renderContext or commandList is null");
        return;
    }

    auto& commandList = renderContext->commandList;
    commandList->SetPipelineState(m_pso);
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetShaderResourceView(
        static_cast<uint32_t>(RootParameterType::Texture),
        0,
        m_gpuCubemap,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // 36 procedural vertices (SV_VertexID), no vertex/index buffer needed.
    commandList->Draw(36, 1, 0, 0);
}
