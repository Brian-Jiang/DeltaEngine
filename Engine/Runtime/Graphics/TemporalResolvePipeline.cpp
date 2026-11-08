#include "Runtime/Graphics/TemporalResolvePipeline.h"

#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/PipelineStateObject.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/ShaderCompile.h"
#include "Runtime/Graphics/TemporalHistory.h"
#include "Runtime/Logging/LogChannels.h"

#include <d3d12.h>
#include <d3dx12.h>

#include <filesystem>

using namespace DeltaEngine;

namespace
{
constexpr const char* kTaaShaderPath = "Shaders/TAA.slang";
constexpr UINT kTaaTextureCount = 4;
}

bool TemporalResolvePipeline::Ensure(Device& device)
{
    if (m_rootSignature && m_pso)
        return true;
    if (m_failed)
        return false;
    m_failed = true;

    Slang::ComPtr<ISlangBlob> vsBlob =
        CompileSlangStage(std::filesystem::path(kTaaShaderPath), "VSMain", "vs_6_6", "TAA VS");
    Slang::ComPtr<ISlangBlob> psBlob =
        CompileSlangStage(std::filesystem::path(kTaaShaderPath), "PSMain", "ps_6_6", "TAA PS");
    if (!vsBlob || !psBlob)
    {
        DLOG(LogRenderer, ELogLevel::Error, "TemporalResolvePipeline: failed to compile {}", kTaaShaderPath);
        return false;
    }

    CD3DX12_DESCRIPTOR_RANGE1 textureRange{};
    textureRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kTaaTextureCount, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 rootParams[static_cast<UINT>(TemporalResolveRootParameter::Count)]{};
    rootParams[static_cast<UINT>(TemporalResolveRootParameter::Textures)].InitAsDescriptorTable(
        1, &textureRange, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParams[static_cast<UINT>(TemporalResolveRootParameter::Constants)].InitAsConstants(
        sizeof(TemporalResolveConstants) / sizeof(uint32_t), 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC samplers[2]{};
    samplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    samplers[1] = CD3DX12_STATIC_SAMPLER_DESC(1, D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
    samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    const D3D12_ROOT_SIGNATURE_FLAGS flags =
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
    rsDesc.Init_1_1(_countof(rootParams), rootParams, _countof(samplers), samplers, flags);

    auto rootSignature = device.CreateRootSignature(rsDesc.Desc_1_1);
    if (!rootSignature)
    {
        DLOG(LogRenderer, ELogLevel::Error, "TemporalResolvePipeline: CreateRootSignature failed");
        return false;
    }
    rootSignature->GetD3D12RootSignature()->SetName(L"RootSignature TemporalResolve");

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
    rtvFormats.RTFormats[0] = kTemporalHistoryFormat;

    pss.pRootSignature = rootSignature->GetD3D12RootSignature().Get();
    pss.VS = D3D12_SHADER_BYTECODE{ vsBlob->getBufferPointer(), vsBlob->getBufferSize() };
    pss.PS = D3D12_SHADER_BYTECODE{ psBlob->getBufferPointer(), psBlob->getBufferSize() };
    pss.RasterizerState = rasterizerState;
    pss.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pss.DepthStencilState = depthStencilState;
    pss.InputLayout = { nullptr, 0 };
    pss.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pss.DSVFormat = DXGI_FORMAT_UNKNOWN;
    pss.RTVFormats = rtvFormats;
    pss.SampleDesc = DXGI_SAMPLE_DESC{ 1, 0 };

    auto pso = device.CreatePipelineStateObject(pss);
    if (!pso)
    {
        DLOG(LogRenderer, ELogLevel::Error, "TemporalResolvePipeline: CreatePipelineStateObject failed");
        return false;
    }
    pso->GetD3D12PipelineState()->SetName(L"PSO TemporalResolve");

    m_rootSignature = std::move(rootSignature);
    m_pso = std::move(pso);
    m_failed = false;
    return true;
}

void TemporalResolvePipeline::Shutdown()
{
    m_pso.reset();
    m_rootSignature.reset();
    m_failed = false;
}
