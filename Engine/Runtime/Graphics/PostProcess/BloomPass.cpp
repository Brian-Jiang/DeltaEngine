#include "Runtime/Graphics/PostProcess/BloomPass.h"

#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/PipelineStateObject.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/RenderGraph/TransientTexturePool.h"
#include "Runtime/Graphics/ShaderCompile.h"
#include "Runtime/Logging/LogChannels.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <algorithm>
#include <cwchar>
#include <string_view>

using namespace DeltaEngine;

namespace
{
constexpr std::string_view kBloomShaderRelativePath = "Shaders/PostProcess_Bloom.slang";

bool ShaderBlobValid(ISlangBlob* blob)
{
    return blob && blob->getBufferPointer() != nullptr && blob->getBufferSize() > 0;
}

std::shared_ptr<RootSignature> CreateSingleSrvRootSignature(Device& device, const wchar_t* name)
{
    CD3DX12_DESCRIPTOR_RANGE1 srvRange{};
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 rootParams[2]{};
    rootParams[0].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[1].InitAsConstants(4, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC linearSampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    linearSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_FLAGS flags =
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init_1_1(_countof(rootParams), rootParams, 1, &linearSampler, flags);

    auto rootSignature = device.CreateRootSignature(rsDesc.Desc_1_1);
    if (rootSignature)
        rootSignature->GetD3D12RootSignature()->SetName(name);
    return rootSignature;
}

std::shared_ptr<RootSignature> CreateCompositeRootSignature(Device& device)
{
    CD3DX12_DESCRIPTOR_RANGE1 srvRange{};
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 rootParams[2]{};
    rootParams[0].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[1].InitAsConstants(4, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC linearSampler(
        0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    linearSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_FLAGS flags =
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init_1_1(_countof(rootParams), rootParams, 1, &linearSampler, flags);

    auto rootSignature = device.CreateRootSignature(rsDesc.Desc_1_1);
    if (rootSignature)
        rootSignature->GetD3D12RootSignature()->SetName(L"RootSignature PostProcess Bloom Composite");
    return rootSignature;
}
} // namespace

void BloomPass::Initialize(Device& /*device*/)
{
}

std::shared_ptr<PipelineStateObject> BloomPass::CreatePSO(Device& device, ISlangBlob* vsBlob, ISlangBlob* psBlob,
    const wchar_t* name) const
{
    struct PipelineStateStream
    {
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
    } pss{};

    CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);
    rasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    CD3DX12_DEPTH_STENCIL_DESC depthStencilState(D3D12_DEFAULT);
    depthStencilState.DepthEnable = FALSE;
    depthStencilState.StencilEnable = FALSE;

    D3D12_RT_FORMAT_ARRAY rtvFormats{};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    D3D12_SHADER_BYTECODE vsBytecode{ vsBlob->getBufferPointer(), vsBlob->getBufferSize() };
    D3D12_SHADER_BYTECODE psBytecode{ psBlob->getBufferPointer(), psBlob->getBufferSize() };

    const bool isComposite = name != nullptr && wcsstr(name, L"Composite") != nullptr;
    pss.pRootSignature =
        (isComposite ? m_compositeRootSignature : m_rootSignature)->GetD3D12RootSignature().Get();
    pss.VS = vsBytecode;
    pss.PS = psBytecode;
    pss.RasterizerState = rasterizerState;
    pss.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pss.DepthStencilState = depthStencilState;
    pss.InputLayout = { nullptr, 0 };
    pss.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pss.DSVFormat = DXGI_FORMAT_UNKNOWN;
    pss.RTVFormats = rtvFormats;
    pss.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };

    auto pso = device.CreatePipelineStateObject(pss);
    if (pso && name)
        pso->GetD3D12PipelineState()->SetName(name);
    return pso;
}

void BloomPass::LazyInitialize(DXGraphicsContext& ctx)
{
    m_initialized = false;
    if (!DELTA_ENSURE_MSG(ctx.device, "BloomPass::LazyInitialize requires non-null ctx.device (pass='{}')",
            m_passName))
        return;

    Device& device = *ctx.device;
    const std::filesystem::path shaderPath(kBloomShaderRelativePath);

    m_vertexShaderBlob = CompileSlangStage(shaderPath, "VSMain", "vs_6_6", "Bloom VSMain");
    Slang::ComPtr<ISlangBlob> extractPsBlob = CompileSlangStage(shaderPath, "PSExtract", "ps_6_6", "Bloom PSExtract");
    Slang::ComPtr<ISlangBlob> blurPsBlob = CompileSlangStage(shaderPath, "PSBlur", "ps_6_6", "Bloom PSBlur");
    Slang::ComPtr<ISlangBlob> compositePsBlob =
        CompileSlangStage(shaderPath, "PSComposite", "ps_6_6", "Bloom PSComposite");

    if (!ShaderBlobValid(m_vertexShaderBlob) || !ShaderBlobValid(extractPsBlob) || !ShaderBlobValid(blurPsBlob)
        || !ShaderBlobValid(compositePsBlob))
    {
        DLOG(LogPostProcess, ELogLevel::Error,
            "BloomPass::LazyInitialize shader compile failed (pass='{}', path='{}')",
            m_passName, shaderPath.string());
        return;
    }

    m_rootSignature = CreateSingleSrvRootSignature(device, L"RootSignature PostProcess Bloom");
    m_compositeRootSignature = CreateCompositeRootSignature(device);
    if (!m_rootSignature || !m_compositeRootSignature)
        return;

    m_extractPso = CreatePSO(device, m_vertexShaderBlob, extractPsBlob, L"PSO PostProcess Bloom Extract");
    m_blurPso = CreatePSO(device, m_vertexShaderBlob, blurPsBlob, L"PSO PostProcess Bloom Blur");
    m_compositePso = CreatePSO(device, m_vertexShaderBlob, compositePsBlob, L"PSO PostProcess Bloom Composite");

    if (!m_extractPso || !m_blurPso || !m_compositePso)
    {
        m_extractPso.reset();
        m_blurPso.reset();
        m_compositePso.reset();
        m_rootSignature.reset();
        m_compositeRootSignature.reset();
        return;
    }

    m_initialized = true;
    m_loggedExecuteSkip = false;
}

void BloomPass::Shutdown()
{
    m_extractPso.reset();
    m_blurPso.reset();
    m_compositePso.reset();
    m_rootSignature.reset();
    m_compositeRootSignature.reset();
    m_vertexShaderBlob = nullptr;
    ReleaseFallbackShader();
    m_initialized = false;
    m_loggedExecuteSkip = false;
}

void BloomPass::DrawFullscreen(DXGraphicsContext& ctx, const std::shared_ptr<PipelineStateObject>& pso,
    D3D12_CPU_DESCRIPTOR_HANDLE outputRTV, UINT width, UINT height, const BloomParamsGPU& params,
    D3D12_CPU_DESCRIPTOR_HANDLE srv0, D3D12_CPU_DESCRIPTOR_HANDLE srv1) const
{
    auto& cl = *ctx.commandList;
    const bool isComposite = srv1.ptr != 0;

    cl.GetD3D12CommandList()->OMSetRenderTargets(1, &outputRTV, FALSE, nullptr);
    cl.SetGraphicsRootSignature(isComposite ? m_compositeRootSignature : m_rootSignature);
    cl.SetPipelineState(pso);
    cl.SetShaderResourceView(0u, 0u, srv0);
    if (isComposite)
        cl.SetShaderResourceView(0u, 1u, srv1);
    cl.SetGraphics32BitConstants(1u, params);

    D3D12_VIEWPORT vp{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    D3D12_RECT sc{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    cl.SetViewport(vp);
    cl.SetScissorRect(sc);
    cl.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cl.Draw(3u, 1u, 0u, 0u);
}

void BloomPass::Execute(DXGraphicsContext& ctx,
                        D3D12_CPU_DESCRIPTOR_HANDLE inputSRV,
                        D3D12_CPU_DESCRIPTOR_HANDLE outputRTV,
                        UINT width, UINT height)
{
    if (!m_initialized)
        LazyInitialize(ctx);

    if (!m_initialized || !ctx.commandList || !ctx.renderManager)
    {
        if (!m_loggedExecuteSkip)
        {
            m_loggedExecuteSkip = true;
            DLOG(LogPostProcess, ELogLevel::Warning,
                "BloomPass::Execute skipping draw (pass='{}', initialized={}, commandList={}, renderManager={})",
                m_passName, m_initialized, static_cast<const void*>(ctx.commandList.get()),
                static_cast<const void*>(ctx.renderManager.get()));
        }
        return;
    }

    const UINT halfWidth = std::max(1u, width / 2);
    const UINT halfHeight = std::max(1u, height / 2);
    const int blurIterations = std::clamp(m_blurIterations, 1, 8);

    auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        halfWidth,
        halfHeight,
        1,
        1,
        1,
        0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    TransientTexturePool& pool = ctx.renderManager->GetTransientPool();
    std::shared_ptr<DirectX12Texture> bloomA = pool.Acquire(desc, "BloomHalfA");
    std::shared_ptr<DirectX12Texture> bloomB = pool.Acquire(desc, "BloomHalfB");
    if (!bloomA || !bloomB)
        return;

    auto& cl = *ctx.commandList;

    BloomParamsGPU extractParams{};
    extractParams.threshold = m_threshold;
    extractParams.softKnee = m_softKnee;

    cl.TransitionBarrier(bloomA, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);
    DrawFullscreen(ctx, m_extractPso, bloomA->GetRenderTargetView(), halfWidth, halfHeight, extractParams, inputSRV);
    cl.TransitionBarrier(bloomA, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        true);

    std::shared_ptr<DirectX12Texture> readTex = bloomA;
    std::shared_ptr<DirectX12Texture> writeTex = bloomB;

    for (int i = 0; i < blurIterations; ++i)
    {
        BloomParamsGPU blurParams{};
        blurParams.texelSize = (static_cast<float>(i) + 0.5f) / static_cast<float>(halfWidth);

        cl.TransitionBarrier(writeTex, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            true);
        DrawFullscreen(ctx, m_blurPso, writeTex->GetRenderTargetView(), halfWidth, halfHeight, blurParams,
            readTex->GetShaderResourceView());
        cl.TransitionBarrier(writeTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, true);

        std::swap(readTex, writeTex);
    }

    BloomParamsGPU compositeParams{};
    compositeParams.intensity = m_intensity;

    DrawFullscreen(ctx, m_compositePso, outputRTV, width, height, compositeParams, inputSRV,
        readTex->GetShaderResourceView());
}
