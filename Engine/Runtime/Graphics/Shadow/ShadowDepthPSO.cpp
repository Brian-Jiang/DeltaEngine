#include "Runtime/Graphics/Shadow/ShadowDepthPSO.h"

#include "Runtime/Graphics/ShaderCompile.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/PipelineStateObject.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/Structures/Vertex.h"
#include "Runtime/Logging/LogChannels.h"

#include <d3dx12.h>

#include <filesystem>
#include <stdexcept>

using namespace DeltaEngine;

namespace
{
std::shared_ptr<PipelineStateObject> BuildShadowDepthPSO(Device& device,
    const std::shared_ptr<RootSignature>& rootSig,
    CD3DX12_SHADER_BYTECODE vs,
    float slopeScaledDepthBias,
    const wchar_t* debugName)
{
    if (!rootSig)
        return {};

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 0;

    CD3DX12_RASTERIZER_DESC rasterizer(D3D12_DEFAULT);
    rasterizer.SlopeScaledDepthBias = slopeScaledDepthBias;

    CD3DX12_DEPTH_STENCIL_DESC depthStencil(D3D12_DEFAULT);
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    CD3DX12_BLEND_DESC blend(D3D12_DEFAULT);

    D3D12_INPUT_ELEMENT_DESC inputElements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    CD3DX12_SHADER_BYTECODE ps {};

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSig;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopology;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
        CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;
        CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
        CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC Blend;
    } stream = {};

    stream.RootSig = rootSig->GetD3D12RootSignature().Get();
    stream.InputLayout = { inputElements, 1 };
    stream.PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    stream.VS = vs;
    stream.PS = ps;
    stream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    stream.RTVFormats = rtvFormats;
    DXGI_SAMPLE_DESC sample { 1, 0 };
    stream.SampleDesc = sample;
    stream.DepthStencil = depthStencil;
    stream.Rasterizer = rasterizer;
    stream.Blend = blend;

    auto pso = device.CreatePipelineStateObject(stream);
    if (pso && debugName)
        pso->GetD3D12PipelineState()->SetName(debugName);
    return pso;
}
}

ShadowDepthPSO::ShadowDepthPSO(Device& device)
{
    D3D12_ROOT_SIGNATURE_FLAGS rootFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_PARAMETER1 rootParameters[ShadowDepthRS::NumParameters] {};
    rootParameters[ShadowDepthRS::ViewProjCB].InitAsConstantBufferView(
        0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[ShadowDepthRS::WorldMatrixCB].InitAsConstantBufferView(
        1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc(ShadowDepthRS::NumParameters,
        rootParameters, 0, nullptr, rootFlags);
    m_rootSignature = device.CreateRootSignature(rootSigDesc.Desc_1_1);
    if (!m_rootSignature)
    {
        DLOG(LogShadow, ELogLevel::Error,
            "ShadowDepthPSO failed: CreateRootSignature returned nullptr (expected valid shadow depth root signature)");
        throw std::runtime_error("ShadowDepthPSO: CreateRootSignature failed");
    }

    Slang::ComPtr<ISlangBlob> vsBlob =
        CompileSlangStage(std::filesystem::path("Shaders/ShadowDepth.slang"), "VSMain", "vs_6_6", "ShadowDepthPSO");
    if (!vsBlob || vsBlob->getBufferSize() == 0)
    {
        DLOG(LogShadow, ELogLevel::Error,
            "ShadowDepthPSO failed: CompileSlangStage returned empty VS blob for '{}' entry '{}' (expected non-empty bytecode)",
            "Shaders/ShadowDepth.slang", "VSMain");
        throw std::runtime_error("ShadowDepthPSO: failed to compile ShadowDepth.slang VSMain");
    }

    CD3DX12_SHADER_BYTECODE vsBytecode(const_cast<void*>(vsBlob->getBufferPointer()),
        static_cast<SIZE_T>(vsBlob->getBufferSize()));

    m_pso2D = BuildShadowDepthPSO(device, m_rootSignature, vsBytecode, 2.0f, L"PSO ShadowDepth 2D");
    m_psoCube = BuildShadowDepthPSO(device, m_rootSignature, vsBytecode, 3.0f, L"PSO ShadowDepth Cube");

    if (!m_pso2D || !m_psoCube)
    {
        DLOG(LogShadow, ELogLevel::Error,
            "ShadowDepthPSO failed: PSO build returned null (pso2D={}, psoCube={}, expected both valid)",
            static_cast<const void*>(m_pso2D.get()), static_cast<const void*>(m_psoCube.get()));
        throw std::runtime_error("ShadowDepthPSO: CreatePipelineStateObject failed");
    }
}
