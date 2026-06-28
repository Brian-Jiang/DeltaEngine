#include "Runtime/Graphics/PostProcess/ColorGradingPass.h"

#include "Runtime/Core/DShader.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/PipelineStateObject.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <dxcapi.h>
#include <wrl/client.h>

#include <string_view>

using namespace Microsoft::WRL;
using namespace DeltaEngine;

namespace
{
constexpr std::string_view kColorGradingShaderRelativePath = "Shaders/PostProcess_ColorGrading.slang";

struct ColorGradingConstants
{
    float saturation;
    float contrast;
    float padding0;
    float padding1;
    DirectX::XMFLOAT4 lift;
    DirectX::XMFLOAT4 gamma;
    DirectX::XMFLOAT4 gain;
};

constexpr UINT kRootConstantCount = static_cast<UINT>(sizeof(ColorGradingConstants) / sizeof(UINT));
static_assert(sizeof(ColorGradingConstants) % sizeof(UINT) == 0);
}

void ColorGradingPass::Initialize(Device& /*device*/)
{
}

void ColorGradingPass::LazyInitialize(DXGraphicsContext& ctx)
{
    m_initialized = false;
    if (!DELTA_ENSURE_MSG(ctx.device,
            "ColorGradingPass::LazyInitialize requires non-null ctx.device (pass='{}', expected shared Device)",
            m_passName))
        return;

    Device& device = *ctx.device;

    DShader* shader = ResolveShader(std::filesystem::path(kColorGradingShaderRelativePath), "VSMain", "PSMain",
        "vs_6_6", "ps_6_6");
    if (!shader)
        return;

    ISlangBlob* vsBlob = shader->GetVertexShaderBlob();
    ISlangBlob* psBlob = shader->GetPixelShaderBlob();
    if (!DELTA_ENSURE_MSG(vsBlob && vsBlob->getBufferPointer() && vsBlob->getBufferSize() > 0,
            "ColorGradingPass VS blob missing after ResolveShader (pass='{}', path='{}', expected non-empty blob)",
            m_passName, std::string(kColorGradingShaderRelativePath)))
        return;
    if (!DELTA_ENSURE_MSG(psBlob && psBlob->getBufferPointer() && psBlob->getBufferSize() > 0,
            "ColorGradingPass PS blob missing after ResolveShader (pass='{}', path='{}', expected non-empty blob)",
            m_passName, std::string(kColorGradingShaderRelativePath)))
        return;

    CD3DX12_DESCRIPTOR_RANGE1 srvRange{};
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 rootParams[2]{};
    rootParams[0].InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[1].InitAsConstants(kRootConstantCount, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

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

    m_rootSignature = device.CreateRootSignature(rsDesc.Desc_1_1);
    if (!DELTA_ENSURE_MSG(m_rootSignature,
            "ColorGradingPass CreateRootSignature failed (pass='{}', expected valid root signature)",
            m_passName))
        return;

    m_rootSignature->GetD3D12RootSignature()->SetName(L"RootSignature PostProcess ColorGrading");

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

    DXGI_SAMPLE_DESC sampleDesc{ 1, 0 };

    D3D12_SHADER_BYTECODE vsBytecode{ vsBlob->getBufferPointer(), vsBlob->getBufferSize() };
    D3D12_SHADER_BYTECODE psBytecode{ psBlob->getBufferPointer(), psBlob->getBufferSize() };

    pss.pRootSignature = m_rootSignature->GetD3D12RootSignature().Get();
    pss.VS = vsBytecode;
    pss.PS = psBytecode;
    pss.RasterizerState = rasterizerState;
    pss.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pss.DepthStencilState = depthStencilState;
    pss.InputLayout = { nullptr, 0 };
    pss.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pss.DSVFormat = DXGI_FORMAT_UNKNOWN;
    pss.RTVFormats = rtvFormats;
    pss.SampleDesc = sampleDesc;

    m_pso = device.CreatePipelineStateObject(pss);
    if (!DELTA_ENSURE_MSG(m_pso,
            "ColorGradingPass CreatePipelineStateObject failed (pass='{}', expected valid PSO)",
            m_passName))
    {
        m_rootSignature.reset();
        return;
    }

    m_pso->GetD3D12PipelineState()->SetName(L"PSO PostProcess ColorGrading");
    m_initialized = true;
    m_loggedExecuteSkip = false;
}

void ColorGradingPass::Shutdown()
{
    m_pso.reset();
    m_rootSignature.reset();
    ReleaseFallbackShader();
    m_initialized = false;
    m_loggedExecuteSkip = false;
}

void ColorGradingPass::Execute(DXGraphicsContext& ctx,
                               D3D12_CPU_DESCRIPTOR_HANDLE inputSRV,
                               D3D12_CPU_DESCRIPTOR_HANDLE /*outputRTV*/,
                               UINT width, UINT height)
{
    if (!m_initialized)
        LazyInitialize(ctx);

    if (!m_initialized || !ctx.commandList)
    {
        if (!m_loggedExecuteSkip)
        {
            m_loggedExecuteSkip = true;
            DLOG(LogPostProcess, ELogLevel::Warning,
                "ColorGradingPass::Execute skipping draw (pass='{}', initialized={}, commandList={} expected initialized pass with non-null command list)",
                m_passName, m_initialized, static_cast<const void*>(ctx.commandList.get()));
        }
        return;
    }

    ColorGradingConstants constants{};
    constants.saturation = m_saturation;
    constants.contrast = m_contrast;
    constants.lift = m_lift;
    constants.gamma = m_gamma;
    constants.gain = m_gain;

    auto& cl = *ctx.commandList;

    cl.SetGraphicsRootSignature(m_rootSignature);
    cl.SetPipelineState(m_pso);
    cl.SetShaderResourceView(0u, 0u, inputSRV);
    cl.SetGraphics32BitConstants(1u, constants);

    D3D12_VIEWPORT vp{ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f };
    D3D12_RECT sc{ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    cl.SetViewport(vp);
    cl.SetScissorRect(sc);

    cl.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cl.Draw(3u, 1u, 0u, 0u);
}
