#include "Runtime/Graphics/IBL/IBLBaker.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <wrl/client.h>
#include <algorithm>
#include <cstdint>
#include <filesystem>

#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/DirectX/PipelineStateObject.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/UnorderedAccessView.h"
#include "Runtime/Graphics/DirectX/ShaderResourceView.h"
#include "Runtime/Graphics/ShaderCompile.h"

using namespace DeltaEngine;
using namespace Microsoft::WRL;

DEFINE_LOG_CATEGORY_STATIC(LogIBL);

namespace
{
    constexpr uint32_t kIrradianceSize   = 32;
    constexpr uint32_t kSpecularBaseSize = 128;
    constexpr uint32_t kSpecularMipCount = 7;
    constexpr uint32_t kBrdfLutSize      = 512;
    constexpr uint32_t kSpecularSamples  = 1024;
    constexpr uint32_t kBrdfLutSamples   = 1024;

    struct IrradianceCB
    {
        uint32_t faceIndex;
        uint32_t outputSize;
        uint32_t _pad0;
        uint32_t _pad1;
    };

    struct SpecularCB
    {
        uint32_t faceIndex;
        uint32_t mipLevel;
        uint32_t outputSize;
        float    roughness;
        uint32_t sampleCount;
        uint32_t sourceCubeSize;
        uint32_t _pad0;
        uint32_t _pad1;
    };

    struct BrdfLutCB
    {
        uint32_t outputSize;
        uint32_t sampleCount;
        uint32_t _pad0;
        uint32_t _pad1;
    };

    enum IBLRootParam : uint32_t
    {
        IBL_RP_Constants = 0,
        IBL_RP_SrcSRV    = 1,
        IBL_RP_DstUAV    = 2,
        IBL_RP_Count
    };

    enum BrdfLutRootParam : uint32_t
    {
        BRDF_RP_Constants = 0,
        BRDF_RP_DstUAV    = 1,
        BRDF_RP_Count
    };
}

IBLBaker::IBLBaker()  = default;
IBLBaker::~IBLBaker() = default;

void IBLBaker::Initialize(Device& device)
{
    if (m_initialized)
    {
        DLOG(LogIBL, ELogLevel::Verbose, "IBLBaker::Initialize called when already initialized; ignoring");
        return;
    }

    CompilePipelines(device);
    DELTA_VERIFY_MSG(m_iblRootSig && m_brdfLutRootSig && m_irradiancePSO && m_specularPSO && m_brdfLutPSO,
        "IBLBaker pipelines failed to compile (irradiance={}, specular={}, brdfLut={})",
        m_irradiancePSO != nullptr, m_specularPSO != nullptr, m_brdfLutPSO != nullptr);

    BakeBrdfLut(device);
    m_initialized = true;

    DLOG(LogIBL, ELogLevel::Log, "IBLBaker initialized (BRDF LUT size={})", kBrdfLutSize);
}

void IBLBaker::CompilePipelines(Device& device)
{
    {
        CD3DX12_DESCRIPTOR_RANGE1 srvRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
        CD3DX12_DESCRIPTOR_RANGE1 uavRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

        CD3DX12_ROOT_PARAMETER1 params[IBL_RP_Count];
        params[IBL_RP_Constants].InitAsConstants(8, 0);
        params[IBL_RP_SrcSRV].InitAsDescriptorTable(1, &srvRange);
        params[IBL_RP_DstUAV].InitAsDescriptorTable(1, &uavRange);

        CD3DX12_STATIC_SAMPLER_DESC linearWrap(0,
            D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
        rsDesc.Init_1_1(IBL_RP_Count, params, 1, &linearWrap);
        m_iblRootSig = device.CreateRootSignature(rsDesc.Desc_1_1);
        m_iblRootSig->GetD3D12RootSignature()->SetName(L"RootSignature IBL");
    }

    {
        CD3DX12_DESCRIPTOR_RANGE1 uavRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0,
            D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

        CD3DX12_ROOT_PARAMETER1 params[BRDF_RP_Count];
        params[BRDF_RP_Constants].InitAsConstants(4, 0);
        params[BRDF_RP_DstUAV].InitAsDescriptorTable(1, &uavRange);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rsDesc;
        rsDesc.Init_1_1(BRDF_RP_Count, params, 0, nullptr);
        m_brdfLutRootSig = device.CreateRootSignature(rsDesc.Desc_1_1);
        m_brdfLutRootSig->GetD3D12RootSignature()->SetName(L"RootSignature IBL BrdfLut");
    }

    struct ComputeStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_CS CS;
    };

    auto makeCs = [&](const std::filesystem::path& file, const std::shared_ptr<RootSignature>& rs)
        -> std::shared_ptr<PipelineStateObject>
    {
        Slang::ComPtr<ISlangBlob> blob = CompileSlangStage(file, "CSMain", "cs_6_6", "IBL");
        if (!blob)
        {
            DLOG(LogIBL, ELogLevel::Error,
                "IBL compute shader '{}' failed to compile (entry='CSMain', profile='cs_6_6')",
                file.string());
            return nullptr;
        }

        ComputeStream stream{};
        stream.pRootSignature = rs->GetD3D12RootSignature().Get();
        D3D12_SHADER_BYTECODE bc{ blob->getBufferPointer(), blob->getBufferSize() };
        stream.CS = bc;
        auto pso = device.CreatePipelineStateObject(stream);
        if (!pso)
        {
            DLOG(LogIBL, ELogLevel::Error,
                "IBL compute pipeline state object creation failed for shader '{}'",
                file.string());
        }
        return pso;
    };

    m_irradiancePSO = makeCs("Shaders/IBL_IrradianceConvolve.slang", m_iblRootSig);
    if (m_irradiancePSO) m_irradiancePSO->GetD3D12PipelineState()->SetName(L"PSO IBL IrradianceConvolve");
    m_specularPSO   = makeCs("Shaders/IBL_SpecularPrefilter.slang", m_iblRootSig);
    if (m_specularPSO) m_specularPSO->GetD3D12PipelineState()->SetName(L"PSO IBL SpecularPrefilter");
    m_brdfLutPSO    = makeCs("Shaders/IBL_BrdfLut.slang", m_brdfLutRootSig);
    if (m_brdfLutPSO) m_brdfLutPSO->GetD3D12PipelineState()->SetName(L"PSO IBL BrdfLut");
}

static std::shared_ptr<DirectX12Texture> CreateCubeUavTexture(
    Device& device, uint32_t size, uint32_t mipLevels, std::string_view name)
{
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        size, size,
        6,
        static_cast<UINT16>(mipLevels),
        1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    auto tex = device.CreateTexture(desc, nullptr);
    tex->SetName(name);
    tex->CreateCubemapSRV();
    return tex;
}

void IBLBaker::BakeBrdfLut(Device& device)
{
    if (!DELTA_ENSURE(m_brdfLutPSO && m_brdfLutRootSig))
    {
        DLOG(LogIBL, ELogLevel::Error, "IBLBaker::BakeBrdfLut skipped: BRDF LUT PSO/root signature missing");
        return;
    }

    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        kBrdfLutSize, kBrdfLutSize,
        1, 1,
        1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    auto lut = device.CreateTexture(desc, nullptr);
    lut->SetName("IBL_BrdfLut");

    auto& queue = device.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto cl     = queue.GetCommandList();
    cl->GetD3D12CommandList()->SetName(L"CommandList IBL BrdfLut");

    cl->TransitionBarrier(lut, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    cl->SetPipelineState(m_brdfLutPSO);
    cl->SetComputeRootSignature(m_brdfLutRootSig);

    BrdfLutCB cb{ kBrdfLutSize, kBrdfLutSamples, 0, 0 };
    cl->SetCompute32BitConstants(BRDF_RP_Constants, cb);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format               = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice   = 0;
    uavDesc.Texture2D.PlaneSlice = 0;

    auto uav = device.CreateUnorderedAccessView(lut, nullptr, &uavDesc);
    cl->SetUnorderedAccessView(BRDF_RP_DstUAV, 0, uav, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0, 0);

    cl->Dispatch((kBrdfLutSize + 7) / 8, (kBrdfLutSize + 7) / 8, 1);
    cl->UAVBarrier(lut);
    cl->TransitionBarrier(lut,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    uint64_t fence = queue.ExecuteCommandList(cl);
    queue.WaitForFenceValue(fence);

    m_staticLut.brdfLut    = lut;
    m_staticLut.brdfLutSRV = lut->GetShaderResourceView();
}

void IBLBaker::BakeIrradiance(Device& device,
                              const std::shared_ptr<DirectX12Texture>& sourceCube,
                              IBLResources& out)
{
    if (!DELTA_ENSURE(m_irradiancePSO && m_iblRootSig))
    {
        DLOG(LogIBL, ELogLevel::Error, "IBLBaker::BakeIrradiance skipped: irradiance PSO/root signature missing");
        return;
    }
    if (!DELTA_ENSURE(sourceCube))
    {
        DLOG(LogIBL, ELogLevel::Error, "IBLBaker::BakeIrradiance skipped: source cube is null");
        return;
    }

    auto irradiance = CreateCubeUavTexture(device, kIrradianceSize, 1, "IBL_IrradianceCube");

    auto& queue = device.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto cl     = queue.GetCommandList();
    cl->GetD3D12CommandList()->SetName(L"CommandList IBL Irradiance");

    cl->TransitionBarrier(irradiance, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cl->TransitionBarrier(sourceCube,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    cl->SetPipelineState(m_irradiancePSO);
    cl->SetComputeRootSignature(m_iblRootSig);

    cl->SetShaderResourceView(IBL_RP_SrcSRV, 0, sourceCube,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format                         = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uavDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    uavDesc.Texture2DArray.MipSlice        = 0;
    uavDesc.Texture2DArray.FirstArraySlice = 0;
    uavDesc.Texture2DArray.ArraySize       = 6;
    uavDesc.Texture2DArray.PlaneSlice      = 0;

    auto uav = device.CreateUnorderedAccessView(irradiance, nullptr, &uavDesc);

    for (uint32_t face = 0; face < 6; ++face)
    {
        IrradianceCB cb{ face, kIrradianceSize, 0, 0 };
        cl->SetCompute32BitConstants(IBL_RP_Constants, cb);
        cl->SetUnorderedAccessView(IBL_RP_DstUAV, 0, uav,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0, 0);

        cl->Dispatch((kIrradianceSize + 7) / 8, (kIrradianceSize + 7) / 8, 1);
        cl->UAVBarrier(irradiance);
    }

    cl->TransitionBarrier(irradiance,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    uint64_t fence = queue.ExecuteCommandList(cl);
    queue.WaitForFenceValue(fence);

    out.irradianceCube = irradiance;
    out.irradianceSRV  = irradiance->GetShaderResourceView();
}

void IBLBaker::BakeSpecular(Device& device,
                            const std::shared_ptr<DirectX12Texture>& sourceCube,
                            IBLResources& out)
{
    if (!DELTA_ENSURE(m_specularPSO && m_iblRootSig))
    {
        DLOG(LogIBL, ELogLevel::Error, "IBLBaker::BakeSpecular skipped: specular PSO/root signature missing");
        return;
    }
    if (!DELTA_ENSURE(sourceCube))
    {
        DLOG(LogIBL, ELogLevel::Error, "IBLBaker::BakeSpecular skipped: source cube is null");
        return;
    }

    auto specular = CreateCubeUavTexture(device, kSpecularBaseSize, kSpecularMipCount, "IBL_SpecularCube");

    uint32_t sourceSize = static_cast<uint32_t>(sourceCube->GetD3D12ResourceDesc().Width);

    auto& queue = device.GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto cl     = queue.GetCommandList();
    cl->GetD3D12CommandList()->SetName(L"CommandList IBL Specular");

    cl->TransitionBarrier(specular, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cl->TransitionBarrier(sourceCube,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    cl->SetPipelineState(m_specularPSO);
    cl->SetComputeRootSignature(m_iblRootSig);
    cl->SetShaderResourceView(IBL_RP_SrcSRV, 0, sourceCube,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    for (uint32_t mip = 0; mip < kSpecularMipCount; ++mip)
    {
        uint32_t mipSize = std::max(1u, kSpecularBaseSize >> mip);
        float    roughness = (kSpecularMipCount > 1)
            ? static_cast<float>(mip) / static_cast<float>(kSpecularMipCount - 1)
            : 0.0f;

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
        uavDesc.Format                         = DXGI_FORMAT_R16G16B16A16_FLOAT;
        uavDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Texture2DArray.MipSlice        = mip;
        uavDesc.Texture2DArray.FirstArraySlice = 0;
        uavDesc.Texture2DArray.ArraySize       = 6;
        uavDesc.Texture2DArray.PlaneSlice      = 0;

        auto uav = device.CreateUnorderedAccessView(specular, nullptr, &uavDesc);

        for (uint32_t face = 0; face < 6; ++face)
        {
            SpecularCB cb{};
            cb.faceIndex      = face;
            cb.mipLevel       = mip;
            cb.outputSize     = mipSize;
            cb.roughness      = roughness;
            cb.sampleCount    = kSpecularSamples;
            cb.sourceCubeSize = sourceSize;

            cl->SetCompute32BitConstants(IBL_RP_Constants, cb);
            cl->SetUnorderedAccessView(IBL_RP_DstUAV, 0, uav,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0, 0);

            uint32_t groups = (mipSize + 7) / 8;
            cl->Dispatch(groups, groups, 1);
            cl->UAVBarrier(specular);
        }
    }

    cl->TransitionBarrier(specular,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    uint64_t fence = queue.ExecuteCommandList(cl);
    queue.WaitForFenceValue(fence);

    out.specularCube = specular;
    out.specularSRV  = specular->GetShaderResourceView();
}

IBLBaker::IBLResources IBLBaker::Bake(Device& device,
                                     const std::shared_ptr<DirectX12Texture>& sourceCube)
{
    IBLResources out{};
    if (!m_initialized)
    {
        DLOG(LogIBL, ELogLevel::Warning, "IBLBaker::Bake called before Initialize; returning empty resources");
        return out;
    }
    if (!sourceCube)
    {
        DLOG(LogIBL, ELogLevel::Warning, "IBLBaker::Bake called with null sourceCube; returning empty resources");
        return out;
    }

    BakeIrradiance(device, sourceCube, out);
    BakeSpecular(device, sourceCube, out);

    out.brdfLut    = m_staticLut.brdfLut;
    out.brdfLutSRV = m_staticLut.brdfLutSRV;
    return out;
}

void IBLBaker::Shutdown()
{
    if (m_initialized)
        DLOG(LogIBL, ELogLevel::Log, "IBLBaker shutting down");

    m_staticLut      = {};
    m_brdfLutPSO.reset();
    m_specularPSO.reset();
    m_irradiancePSO.reset();
    m_brdfLutRootSig.reset();
    m_iblRootSig.reset();
    m_initialized = false;
}
