#include "DXRenderManager.h"

#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <dxgidebug.h>
#include <pix3.h>

#include "Graphics/DXUtils.h"
#include "IO/IOManager.h"
#include "Core/Time.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/Adapter.h"
#include "Runtime/Graphics/Structures/RootParameterType.h"
#include "Runtime/Graphics/MaterialConstants.h"
#include "Runtime/Graphics/DefaultTextures.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Graphics/PostProcess/PostProcessPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
#include "Runtime/Graphics/RenderGraph/MsaaResolveGraphPass.h"
#include "Runtime/Graphics/RenderGraph/PostProcessRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/PostProcessFinalizeGraphPass.h"
#include "Runtime/Graphics/RenderGraph/ShadowRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/SceneRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/SceneShadowReadGraphPass.h"
#include "Runtime/Graphics/RenderGraph/SkyboxRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/GBufferRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/GBufferAlbedoBlitGraphPass.h"
#include "Runtime/Graphics/ShaderCompile.h"
#include "Runtime/Graphics/Structures/GBufferRootParameterType.h"
#include "Runtime/Graphics/DirectX/PipelineStateObject.h"
#include "Runtime/Graphics/RenderProxy/CameraRenderProxy.h"
#include "Runtime/Graphics/RenderProxy/SkyboxRenderProxy.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/Skybox.h"
#include "Runtime/Core/DTexture.h"
#include "Runtime/Graphics/Shadow/ShadowConstants.h"
#include "Runtime/Graphics/Shadow/ShadowDepthPSO.h"
#include "Runtime/Graphics/Shadow/ShadowSettings.h"
#include "Runtime/Graphics/RenderResourceReleaseService.h"
#include "Runtime/Graphics/RenderProxy/RenderProxy.h"

#include <algorithm>
#include <filesystem>

using namespace Microsoft::WRL;
using namespace DeltaEngine;
using namespace DirectX;

namespace
{
constexpr RenderPath kActiveRenderPath = RenderPath::Deferred;
}

DXRenderManager::DXRenderManager(std::shared_ptr<Device> device, std::shared_ptr<RenderTarget> renderTarget, UINT width, UINT height)
    : m_device(std::move(device)), m_renderTarget(std::move(renderTarget)), m_renderPath(kActiveRenderPath),
    m_width(width), m_height(height),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
{
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    m_releaseQueue.SetFenceCompleteChecker([this](uint64_t fenceValue)
    {
        return m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).IsFenceComplete(fenceValue);
    });
    GetRenderResourceReleaseService().RegisterQueue(&m_releaseQueue);

    m_transientPool.SetTextureFactory(
        [this](const D3D12_RESOURCE_DESC& desc, const std::string& name)
        {
            auto texture = m_device->CreateTexture(desc, nullptr);
            if (texture)
                texture->SetName(name);
            return texture;
        });
    m_frameGraph.SetTransientPool(&m_transientPool);

    LoadPipeline();
    LoadAssets();
}

DXRenderManager::~DXRenderManager()
{
    GetRenderResourceReleaseService().UnregisterQueue(&m_releaseQueue);
}

void DXRenderManager::LoadPipeline()
{
    // Device and RenderTarget are provided by the caller (Editor/Game)
}

void DXRenderManager::LoadAssets()
{
    DELTA_VERIFY_MSG(m_device && m_renderTarget,
        "DXRenderManager::LoadAssets requires a valid device and render target");

    // todo root signature should bind to pass?
    // ---- Root signature (shared across all renderers) ----
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    

    CD3DX12_ROOT_PARAMETER1 rootParameters[static_cast<UINT>(RootParameterType::NumRootParameterTypes)] {};

    // ==== CBV (b) ====
    // Camera (b0)
    rootParameters[static_cast<UINT>(RootParameterType::CameraCB)].InitAsConstantBufferView(0);
    // Object (b1)
    rootParameters[static_cast<UINT>(RootParameterType::ObjectCB)].InitAsConstantBufferView(1);
    // Light (b2)
    rootParameters[static_cast<UINT>(RootParameterType::LightCB)].InitAsConstantBufferView(2);
    // Material (b3)
    rootParameters[static_cast<UINT>(RootParameterType::MaterialCB)].InitAsConstantBufferView(3);


    // ==== SRV (t) ====
    // Lights (t0, t1, t2)
    rootParameters[static_cast<UINT>(RootParameterType::PointLights)].InitAsShaderResourceView(0);
    rootParameters[static_cast<UINT>(RootParameterType::SpotLights)].InitAsShaderResourceView(1);
    rootParameters[static_cast<UINT>(RootParameterType::DirectionalLights)].InitAsShaderResourceView(2);

    // Material textures (t0..t4, space1)
    CD3DX12_DESCRIPTOR_RANGE1 ranges[3] {};
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, static_cast<UINT>(MaterialTextureSlot::Count), 0, 1,
        D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
    rootParameters[static_cast<UINT>(RootParameterType::Texture)].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

    // IBL textures (t0..t2, space2)
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3u, 0, 2,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    rootParameters[static_cast<UINT>(RootParameterType::IBLTextures)].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

    // Shadow maps (t0..t2, space3)
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3u, 0, 3,
        D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
    rootParameters[static_cast<UINT>(RootParameterType::ShadowMaps)].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

    rootParameters[static_cast<UINT>(RootParameterType::ShadowCB)].InitAsConstantBufferView(4, 0,
        D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);


    // ==== Sampler (s) ====
    // Anisotropic sampler (s0) — scene samplers
    CD3DX12_STATIC_SAMPLER_DESC staticSamplers[4] {};
    staticSamplers[0] = CD3DX12_STATIC_SAMPLER_DESC(0, D3D12_FILTER_ANISOTROPIC);
    // Anisotropic wrap sampler (s1) — material textures
    staticSamplers[1] = CD3DX12_STATIC_SAMPLER_DESC(
        1,
        D3D12_FILTER_ANISOTROPIC,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0.0f,
        16u,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
        0.0f,
        D3D12_FLOAT32_MAX,
        D3D12_SHADER_VISIBILITY_PIXEL);
    // Trilinear clamp sampler (s2) — IBL sampling
    staticSamplers[2] = CD3DX12_STATIC_SAMPLER_DESC(
        2,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
        0.0f,
        0u,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK,
        0.0f,
        D3D12_FLOAT32_MAX,
        D3D12_SHADER_VISIBILITY_PIXEL);
    staticSamplers[3] = CD3DX12_STATIC_SAMPLER_DESC(
        3,
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        0.0f,
        0u,
        D3D12_COMPARISON_FUNC_LESS,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
        0.0f,
        D3D12_FLOAT32_MAX,
        D3D12_SHADER_VISIBILITY_PIXEL);


    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(static_cast<UINT>(RootParameterType::NumRootParameterTypes), rootParameters,
        _countof(staticSamplers), staticSamplers, rootSignatureFlags);

    m_rootSignature = m_device->CreateRootSignature(rootSignatureDescription.Desc_1_1);
    m_rootSignature->GetD3D12RootSignature()->SetName(L"RootSignature Scene");

    InitGBufferPipeline();

    m_iblBaker.Initialize(*m_device);
    m_shadowPass.Initialize(*m_device);
}

void DXRenderManager::InitGBufferPipeline()
{
    m_gbufferVertexShaderBlob = CompileSlangStage(
        std::filesystem::path("Shaders/GBuffer.slang"), "VSMain", "vs_6_6", "GBuffer VS");
    m_gbufferPixelShaderBlob = CompileSlangStage(
        std::filesystem::path("Shaders/GBuffer.slang"), "PSMain", "ps_6_6", "GBuffer PS");

    if (!m_gbufferVertexShaderBlob || !m_gbufferPixelShaderBlob)
        return;

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_PARAMETER1 rootParameters[static_cast<UINT>(GBufferRootParameterType::NumRootParameterTypes)] {};
    rootParameters[static_cast<UINT>(GBufferRootParameterType::CameraCB)].InitAsConstantBufferView(0);
    rootParameters[static_cast<UINT>(GBufferRootParameterType::ObjectCB)].InitAsConstantBufferView(1);
    rootParameters[static_cast<UINT>(GBufferRootParameterType::MaterialCB)].InitAsConstantBufferView(3);

    CD3DX12_DESCRIPTOR_RANGE1 textureRange {};
    textureRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, static_cast<UINT>(MaterialTextureSlot::Count), 0, 1,
        D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
    rootParameters[static_cast<UINT>(GBufferRootParameterType::Texture)].InitAsDescriptorTable(
        1, &textureRange, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC materialSampler(
        1,
        D3D12_FILTER_ANISOTROPIC,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        0.0f,
        16u,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE,
        0.0f,
        D3D12_FLOAT32_MAX,
        D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(
        static_cast<UINT>(GBufferRootParameterType::NumRootParameterTypes),
        rootParameters,
        1,
        &materialSampler,
        rootSignatureFlags);

    m_gbufferRootSignature = m_device->CreateRootSignature(rootSignatureDescription.Desc_1_1);
    if (m_gbufferRootSignature)
        m_gbufferRootSignature->GetD3D12RootSignature()->SetName(L"RootSignature GBuffer");
}

ISlangBlob* DXRenderManager::GetGBufferVertexShaderBlob() const
{
    return m_gbufferVertexShaderBlob.get();
}

ISlangBlob* DXRenderManager::GetGBufferPixelShaderBlob() const
{
    return m_gbufferPixelShaderBlob.get();
}

D3D12_RT_FORMAT_ARRAY DXRenderManager::GetGBufferRTVFormats() const
{
    D3D12_RT_FORMAT_ARRAY formats {};
    formats.NumRenderTargets = 4;
    formats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    formats.RTFormats[1] = DXGI_FORMAT_R16G16B16A16_FLOAT;
    formats.RTFormats[2] = DXGI_FORMAT_R8G8B8A8_UNORM;
    formats.RTFormats[3] = DXGI_FORMAT_R11G11B10_FLOAT;
    return formats;
}

bool DXRenderManager::EnsureGBufferAlbedoBlitPipeline()
{
    if (m_gbufferAlbedoBlitReady)
        return true;

    return InitGBufferAlbedoBlitPipeline();
}

bool DXRenderManager::InitGBufferAlbedoBlitPipeline()
{
    if (!m_device)
        return false;

    Slang::ComPtr<ISlangBlob> vsBlob = CompileSlangStage(
        std::filesystem::path("Shaders/GBufferAlbedoBlit.slang"), "VSMain", "vs_6_6", "GBufferAlbedoBlit VS");
    Slang::ComPtr<ISlangBlob> psBlob = CompileSlangStage(
        std::filesystem::path("Shaders/GBufferAlbedoBlit.slang"), "PSMain", "ps_6_6", "GBufferAlbedoBlit PS");
    if (!vsBlob || !psBlob)
        return false;

    CD3DX12_DESCRIPTOR_RANGE1 srvRange {};
    srvRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);

    CD3DX12_ROOT_PARAMETER1 rootParam {};
    rootParam.InitAsDescriptorTable(1, &srvRange, D3D12_SHADER_VISIBILITY_PIXEL);

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
    rsDesc.Init_1_1(1, &rootParam, 1, &linearSampler, flags);

    m_gbufferAlbedoBlitRootSignature = m_device->CreateRootSignature(rsDesc.Desc_1_1);
    if (!m_gbufferAlbedoBlitRootSignature)
        return false;

    m_gbufferAlbedoBlitRootSignature->GetD3D12RootSignature()->SetName(L"RootSignature GBufferAlbedoBlit");

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
    } pss {};

    CD3DX12_RASTERIZER_DESC rasterizerState(D3D12_DEFAULT);
    rasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    CD3DX12_DEPTH_STENCIL_DESC depthStencilState(D3D12_DEFAULT);
    depthStencilState.DepthEnable = FALSE;
    depthStencilState.StencilEnable = FALSE;

    D3D12_RT_FORMAT_ARRAY rtvFormats {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

    D3D12_SHADER_BYTECODE vsBytecode { vsBlob->getBufferPointer(), vsBlob->getBufferSize() };
    D3D12_SHADER_BYTECODE psBytecode { psBlob->getBufferPointer(), psBlob->getBufferSize() };

    pss.pRootSignature = m_gbufferAlbedoBlitRootSignature->GetD3D12RootSignature().Get();
    pss.VS = vsBytecode;
    pss.PS = psBytecode;
    pss.RasterizerState = rasterizerState;
    pss.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pss.DepthStencilState = depthStencilState;
    pss.InputLayout = { nullptr, 0 };
    pss.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pss.DSVFormat = DXGI_FORMAT_UNKNOWN;
    pss.RTVFormats = rtvFormats;
    pss.SampleDesc = { 1, 0 };

    m_gbufferAlbedoBlitPSO = m_device->CreatePipelineStateObject(pss);
    if (!m_gbufferAlbedoBlitPSO)
        return false;

    m_gbufferAlbedoBlitPSO->GetD3D12PipelineState()->SetName(L"PSO GBufferAlbedoBlit");
    m_gbufferAlbedoBlitReady = true;
    return true;
}

void DXRenderManager::InitWorldRenderers(DWorld& world)
{
    DELTA_VERIFY(m_device);
    DELTA_VERIFY(m_renderTarget);
    DLOG(LogRenderer, ELogLevel::Log, "DXRenderManager initializing world renderers ({}x{})", m_width, m_height);

    // Create a color buffer with sRGB for gamma correction.
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;

    // Check the best multisample quality level that can be used for the given back buffer format.
    DXGI_SAMPLE_DESC sampleDesc{};
    if (m_renderPath == RenderPath::Forward)
        sampleDesc = m_device->GetMultisampleQualityLevels(backBufferFormat);
    else
        sampleDesc = { 1, 0 };

    // Create an off-screen render target with a single color buffer and a depth buffer.
    auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(backBufferFormat, m_width, m_height, 1, 1, sampleDesc.Count,
        sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    D3D12_CLEAR_VALUE colorClearValue;
    colorClearValue.Format = colorDesc.Format;
    colorClearValue.Color[0] = 0.0f;
    colorClearValue.Color[1] = 0.2f;
    colorClearValue.Color[2] = 0.4f;
    colorClearValue.Color[3] = 1.0f;

    auto colorTexture = m_device->CreateTexture(colorDesc, &colorClearValue);
    colorTexture->SetName("Color Render Target");

    // Create a depth buffer.
    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_width, m_height, 1, 1, sampleDesc.Count,
        sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE depthClearValue;
    depthClearValue.Format = depthDesc.Format;
    depthClearValue.DepthStencil = { 1.0f, 0 };

    auto depthTexture = m_device->CreateTexture(depthDesc, &depthClearValue);
    depthTexture->SetName("Depth Render Target");

    m_renderTarget->AttachTexture(AttachmentPoint::Color0, colorTexture);
    m_renderTarget->AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

    CreatePingPongTargets(m_width, m_height);

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = directCommandQueue.GetCommandList();
    m_currentCommandList = commandList;

    DefaultTextures::Initialize(*m_device, *commandList);

    m_currentWorld = &world;

    auto context = GetGraphicsContext();
    world.InitRenderers(context);

    directCommandQueue.ExecuteCommandList(m_currentCommandList);
    m_currentCommandList = nullptr;
    m_currentContext.reset();
}

void DXRenderManager::SetPendingActiveRenderCamera(std::optional<ActiveRenderCamera> camera)
{
    m_pendingActiveRenderCamera = std::move(camera);
}

void FrameGraphBindings::Register(RenderGraphTextureHandle handle,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE srv)
{
    if (!handle.IsValid())
        return;

    if (entries.size() <= handle.index)
        entries.resize(handle.index + 1);

    entries[handle.index] = { rtv, srv };
}

D3D12_CPU_DESCRIPTOR_HANDLE FrameGraphBindings::RtvFor(RenderGraphTextureHandle handle) const
{
    if (!handle.IsValid() || handle.index >= entries.size())
        return {};

    return entries[handle.index].rtv;
}

D3D12_CPU_DESCRIPTOR_HANDLE FrameGraphBindings::SrvFor(RenderGraphTextureHandle handle) const
{
    if (!handle.IsValid() || handle.index >= entries.size())
        return {};

    return entries[handle.index].srv;
}

void DXRenderManager::PrepareFrame()
{
    m_releaseQueue.ProcessCompleted();

    CommandQueue& frameQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    const uint64_t completedFence = frameQueue.GetCompletedFenceValue();
    m_transientPool.BeginFrame(completedFence);
    m_device->ReleaseStaleDescriptors(completedFence);

    DTexture* skyboxCube = nullptr;
    if (m_currentWorld && m_currentWorld->GetSkybox())
        skyboxCube = m_currentWorld->GetSkybox()->m_cubemapTexture;
    UpdateIBL(skyboxCube);

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = directCommandQueue.GetCommandList();
    commandList->GetD3D12CommandList()->SetName(L"CommandList Scene");
    m_currentCommandList = commandList;
    m_currentContext.reset();
    commandList->SetGraphicsRootSignature(m_rootSignature);

    auto ctx = GetGraphicsContext();
    ctx->activeRenderCamera = m_pendingActiveRenderCamera;
    m_pendingActiveRenderCamera.reset();
    if (m_currentWorld)
        m_currentWorld->PreGatherDrawCalls(ctx);
}

void DXRenderManager::RenderScene(const SceneDrawCallback& drawCallback)
{
    m_pendingSceneDrawCallback = drawCallback;
}

void DXRenderManager::BuildFrameGraph(const SceneDrawCallback& drawCallback, PostProcessStack* stack)
{
    switch (m_renderPath)
    {
    case RenderPath::Forward:
        BuildForwardFrameGraph(drawCallback, stack);
        break;
    case RenderPath::Deferred:
        BuildDeferredFrameGraph(drawCallback, stack);
        break;
    }
}

bool DXRenderManager::ImportSceneTargets(RenderGraphTextureUsage colorAndShader,
    std::shared_ptr<DirectX12Texture>& colorTexture, std::shared_ptr<DirectX12Texture>& depthTexture)
{
    m_frameResources = {};
    m_frameBindings = {};

    colorTexture = m_renderTarget->GetTexture(AttachmentPoint::Color0);
    depthTexture = m_renderTarget->GetTexture(AttachmentPoint::DepthStencil);
    if (!colorTexture || !depthTexture)
        return false;

    m_frameResources.sceneColor = m_frameGraph.ImportTexture("SceneColor", colorTexture, colorAndShader);
    m_frameBindings.Register(m_frameResources.sceneColor,
        colorTexture->GetRenderTargetView(), colorTexture->GetShaderResourceView());

    m_frameResources.sceneDepth = m_frameGraph.ImportTexture("SceneDepth", depthTexture,
        RenderGraphTextureUsage::DepthAttachment);
    m_frameBindings.Register(m_frameResources.sceneDepth, depthTexture->GetDepthStencilView(), {});

    return true;
}

void DXRenderManager::AddShadowPasses(RenderGraphTextureUsage depthAndShader)
{
    if (!m_currentWorld || !m_shadowPass.ShadowResourcesReady())
        return;

    auto ctx = GetGraphicsContext();

    m_frameResources.shadowDirectional = m_frameGraph.ImportTexture("ShadowDirectional",
        m_shadowPass.GetDirectionalAtlasTexture(), depthAndShader);
    m_frameResources.shadowSpot = m_frameGraph.ImportTexture("ShadowSpot",
        m_shadowPass.GetSpotAtlasTexture(), depthAndShader);
    m_frameResources.shadowPoint = m_frameGraph.ImportTexture("ShadowPointCubes",
        m_shadowPass.GetPointCubeArrayTexture(), depthAndShader);

    m_frameGraph.AddPass(std::make_unique<ShadowRenderGraphPass>(
        &m_shadowPass, m_currentWorld, ctx,
        m_frameResources.shadowDirectional, m_frameResources.shadowSpot, m_frameResources.shadowPoint));
    m_frameGraph.AddPass(std::make_unique<SceneShadowReadGraphPass>(
        m_frameResources.shadowDirectional, m_frameResources.shadowSpot, m_frameResources.shadowPoint));
}

void DXRenderManager::CreateGBufferTextures(RenderGraphTextureUsage gbufferUsage)
{
    const auto makeDesc = [this](DXGI_FORMAT format)
    {
        return CD3DX12_RESOURCE_DESC::Tex2D(
            format,
            m_width,
            m_height,
            1,
            1,
            1,
            0,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    };

    auto registerTexture = [this, gbufferUsage](const char* name, const D3D12_RESOURCE_DESC& desc)
    {
        const RenderGraphTextureHandle handle = m_frameGraph.CreateTexture(name, desc, gbufferUsage);
        auto texture = m_frameGraph.GetImportedTexture(handle).texture;
        if (texture)
            m_frameBindings.Register(handle, texture->GetRenderTargetView(), texture->GetShaderResourceView());
        return handle;
    };

    m_frameResources.gbufferAlbedo = registerTexture("GBufferAlbedo",
        makeDesc(DXGI_FORMAT_R8G8B8A8_UNORM));
    m_frameResources.gbufferNormal = registerTexture("GBufferNormal",
        makeDesc(DXGI_FORMAT_R16G16B16A16_FLOAT));
    m_frameResources.gbufferMaterial = registerTexture("GBufferMaterial",
        makeDesc(DXGI_FORMAT_R8G8B8A8_UNORM));
    m_frameResources.gbufferEmissive = registerTexture("GBufferEmissive",
        makeDesc(DXGI_FORMAT_R11G11B10_FLOAT));
}

void DXRenderManager::AddShadowSceneSkyboxPasses(const SceneDrawCallback& drawCallback, const float clearColor[4],
    RenderGraphTextureUsage depthAndShader)
{
    auto ctx = GetGraphicsContext();
    const SceneRenderGraphPass::DescriptorStageCallback stageDescriptors =
        [this](CommandList& cl)
        {
            StageIBLDescriptors(cl);
            StageShadowDescriptors(cl);
        };

    AddShadowPasses(depthAndShader);

    m_frameGraph.AddPass(std::make_unique<SceneRenderGraphPass>(
        m_frameResources.sceneColor, m_frameResources.sceneDepth,
        RenderGraphClearValue::Color4(clearColor[0], clearColor[1], clearColor[2], clearColor[3]),
        RenderGraphClearValue::DepthStencil(1.0f),
        m_renderTarget.get(), m_rootSignature, m_viewport, m_scissorRect,
        stageDescriptors, ctx, drawCallback));

    if (m_currentWorld && m_currentWorld->GetSkybox())
    {
        m_frameGraph.AddPass(std::make_unique<SkyboxRenderGraphPass>(
            m_frameResources.sceneColor, m_frameResources.sceneDepth,
            m_renderTarget.get(), m_rootSignature, m_viewport, m_scissorRect,
            stageDescriptors, m_currentWorld, ctx));
    }
}

void DXRenderManager::FinalizeNoPostProcessOutput()
{
    m_frameGraph.AddPass(std::make_unique<PostProcessFinalizeGraphPass>(m_frameResources.sceneColor));
    m_frameResources.finalOutput = m_frameResources.sceneColor;
    m_finalPostProcessSRV = m_frameBindings.SrvFor(m_frameResources.sceneColor);
    m_hasPostProcessedOutput = false;
    m_finalPostProcessTexture.reset();
}

void DXRenderManager::AppendPostProcessChain(PostProcessStack* stack, RenderGraphTextureHandle postInputHandle,
    RenderGraphTextureUsage colorAndShader)
{
    m_frameResources.ping = m_frameGraph.ImportTexture("Ping", m_pingPong[0].texture, colorAndShader);
    m_frameResources.pong = m_frameGraph.ImportTexture("Pong", m_pingPong[1].texture, colorAndShader);
    m_frameBindings.Register(m_frameResources.ping, m_pingPong[0].rtv, m_pingPong[0].srv);
    m_frameBindings.Register(m_frameResources.pong, m_pingPong[1].rtv, m_pingPong[1].srv);

    RenderGraphTextureHandle inputHandle = postInputHandle;
    int writeIdx = 0;
    const int passCount = stack->GetPassCount();
    for (int i = 0; i < passCount; ++i)
    {
        PostProcessPass* pass = stack->GetPass(i);
        if (!pass)
            continue;

        m_trackedPasses.insert(pass);

        const RenderGraphTextureHandle outputHandle = writeIdx == 0 ? m_frameResources.ping : m_frameResources.pong;
        m_frameGraph.AddPass(std::make_unique<PostProcessRenderGraphPass>(
            pass, inputHandle, outputHandle,
            m_frameBindings.SrvFor(inputHandle), m_frameBindings.RtvFor(outputHandle),
            m_width, m_height));

        inputHandle = outputHandle;
        writeIdx = 1 - writeIdx;
    }

    m_frameGraph.AddPass(std::make_unique<PostProcessFinalizeGraphPass>(inputHandle));
    m_frameResources.finalOutput = inputHandle;
    m_finalPostProcessSRV = m_frameBindings.SrvFor(inputHandle);
    m_hasPostProcessedOutput = true;
    m_finalPostProcessTexture = m_frameGraph.GetImportedTexture(inputHandle).texture;
}

void DXRenderManager::BuildForwardFrameGraph(const SceneDrawCallback& drawCallback, PostProcessStack* stack)
{
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;
    const RenderGraphTextureUsage depthAndShader =
        RenderGraphTextureUsage::DepthAttachment | RenderGraphTextureUsage::ShaderResource;

    std::shared_ptr<DirectX12Texture> colorTexture;
    std::shared_ptr<DirectX12Texture> depthTexture;
    if (!ImportSceneTargets(colorAndShader, colorTexture, depthTexture))
        return;

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    AddShadowSceneSkyboxPasses(drawCallback, clearColor, depthAndShader);

    if (!stack || stack->GetPassCount() == 0)
    {
        FinalizeNoPostProcessOutput();
        return;
    }

    RenderGraphTextureHandle postInputHandle = m_frameResources.sceneColor;

    const auto sceneDesc = colorTexture->GetD3D12ResourceDesc();
    const bool useResolvedScene = sceneDesc.SampleDesc.Count > 1;
    if (useResolvedScene)
    {
        const auto resolvedDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            sceneDesc.Format, sceneDesc.Width, static_cast<UINT>(sceneDesc.Height),
            1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE);
        m_frameResources.resolvedScene = m_frameGraph.CreateTexture("ResolvedScene", resolvedDesc,
            RenderGraphTextureUsage::ShaderResource);
        auto resolvedScene = m_frameGraph.GetImportedTexture(m_frameResources.resolvedScene).texture;
        if (resolvedScene)
        {
            m_frameBindings.Register(m_frameResources.resolvedScene, {}, resolvedScene->GetShaderResourceView());
            m_frameGraph.AddPass(std::make_unique<MsaaResolveGraphPass>(
                m_frameResources.sceneColor, m_frameResources.resolvedScene, colorTexture, resolvedScene));
            postInputHandle = m_frameResources.resolvedScene;
        }
    }

    AppendPostProcessChain(stack, postInputHandle, colorAndShader);
}

void DXRenderManager::BuildDeferredFrameGraph(const SceneDrawCallback& drawCallback, PostProcessStack* stack)
{
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;
    const RenderGraphTextureUsage depthAndShader =
        RenderGraphTextureUsage::DepthAttachment | RenderGraphTextureUsage::ShaderResource;
    const RenderGraphTextureUsage gbufferUsage =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    std::shared_ptr<DirectX12Texture> colorTexture;
    std::shared_ptr<DirectX12Texture> depthTexture;
    if (!ImportSceneTargets(colorAndShader, colorTexture, depthTexture))
        return;

    auto ctx = GetGraphicsContext();

    AddShadowPasses(depthAndShader);
    CreateGBufferTextures(gbufferUsage);

    m_frameGraph.AddPass(std::make_unique<GBufferRenderGraphPass>(
        m_frameResources.gbufferAlbedo,
        m_frameResources.gbufferNormal,
        m_frameResources.gbufferMaterial,
        m_frameResources.gbufferEmissive,
        m_frameResources.sceneDepth,
        m_frameBindings.RtvFor(m_frameResources.gbufferAlbedo),
        m_frameBindings.RtvFor(m_frameResources.gbufferNormal),
        m_frameBindings.RtvFor(m_frameResources.gbufferMaterial),
        m_frameBindings.RtvFor(m_frameResources.gbufferEmissive),
        depthTexture ? depthTexture->GetDepthStencilView() : D3D12_CPU_DESCRIPTOR_HANDLE {},
        m_gbufferRootSignature,
        m_viewport,
        m_scissorRect,
        ctx,
        drawCallback));

    m_frameGraph.AddPass(std::make_unique<GBufferAlbedoBlitGraphPass>(
        m_frameResources.gbufferAlbedo,
        m_frameResources.sceneColor,
        m_frameBindings.SrvFor(m_frameResources.gbufferAlbedo),
        m_frameBindings.RtvFor(m_frameResources.sceneColor),
        m_viewport,
        m_scissorRect,
        this));

    if (!stack || stack->GetPassCount() == 0)
    {
        FinalizeNoPostProcessOutput();
        return;
    }

    AppendPostProcessChain(stack, m_frameResources.sceneColor, colorAndShader);
}

void DXRenderManager::ExecuteBootstrapSceneFallback(DXGraphicsContext& ctx,
    const SceneDrawCallback& drawCallback)
{
    if (!ctx.commandList)
        return;

    auto& commandList = *ctx.commandList;
    auto colorTexture = m_renderTarget->GetTexture(AttachmentPoint::Color0);
    auto depthTexture = m_renderTarget->GetTexture(AttachmentPoint::DepthStencil);

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    if (colorTexture)
        commandList.ClearTexture(colorTexture, clearColor);
    if (depthTexture)
        commandList.ClearDepthStencilTexture(depthTexture, D3D12_CLEAR_FLAG_DEPTH);

    commandList.SetViewport(m_viewport);
    commandList.SetScissorRect(m_scissorRect);
    commandList.SetRenderTarget(*m_renderTarget);
    commandList.SetGraphicsRootSignature(m_rootSignature);

    StageIBLDescriptors(commandList);
    StageShadowDescriptors(commandList);

    if (drawCallback && m_currentContext)
        drawCallback(m_currentContext);
}

void DXRenderManager::ExecuteFrameGraph(DXGraphicsContext& ctx, const SceneDrawCallback& drawCallback)
{
    auto colorTexture = m_renderTarget->GetTexture(AttachmentPoint::Color0);
    auto depthTexture = m_renderTarget->GetTexture(AttachmentPoint::DepthStencil);

    if (!colorTexture || !depthTexture)
    {
        if (m_currentWorld && !m_shadowPass.ShadowResourcesReady() && m_currentContext)
            m_shadowPass.Render(m_currentContext, *m_currentWorld);

        ExecuteBootstrapSceneFallback(ctx, drawCallback);

        auto sceneTex = m_renderTarget->GetTexture(AttachmentPoint::Color0);
        if (sceneTex)
            m_finalPostProcessSRV = sceneTex->GetShaderResourceView();
        m_hasPostProcessedOutput = false;
        m_finalPostProcessTexture.reset();
        return;
    }

    if (m_currentWorld && !m_shadowPass.ShadowResourcesReady() && m_currentContext)
        m_shadowPass.Render(m_currentContext, *m_currentWorld);

    if (m_frameGraph.GetPassCount() == 0)
        return;

    PIXBeginEvent(ctx.commandList->GetD3D12CommandList().Get(), PIX_COLOR_DEFAULT, L"FrameGraph");
    m_frameGraph.Execute({ ctx.commandList.get(), &ctx });
    PIXEndEvent(ctx.commandList->GetD3D12CommandList().Get());
}

const ShadowDepthPSO* DXRenderManager::GetShadowDepthPSO() const
{
    return m_shadowPass.GetShadowDepthPSO();
}

void DXRenderManager::EnsureIBLFallback()
{
    if (m_iblFallbackReady)
        return;

    auto blackCube = DefaultTextures::GetBlackCubeTexture();
    auto blackRG   = DefaultTextures::GetBlackRGTexture();
    if (!blackCube || !blackRG)
        return;

    m_iblResources.irradianceCube = blackCube;
    m_iblResources.specularCube   = blackCube;
    m_iblResources.brdfLut        = m_iblBaker.GetStaticLut().brdfLut ? m_iblBaker.GetStaticLut().brdfLut : blackRG;
    m_iblResources.irradianceSRV  = blackCube->GetShaderResourceView();
    m_iblResources.specularSRV    = blackCube->GetShaderResourceView();
    m_iblResources.brdfLutSRV     = m_iblBaker.GetStaticLut().brdfLutSRV.ptr
        ? m_iblBaker.GetStaticLut().brdfLutSRV
        : blackRG->GetShaderResourceView();
    m_iblFallbackReady = true;
}

void DXRenderManager::UpdateIBL(DTexture* skyboxCubemap)
{
    if (skyboxCubemap == m_lastSkyboxTexture && (m_lastSkyboxTexture != nullptr || m_iblFallbackReady))
        return;

    if (!skyboxCubemap)
    {
        EnsureIBLFallback();
        m_lastSkyboxTexture = nullptr;
        return;
    }

    Skybox* skybox = m_currentWorld ? m_currentWorld->GetSkybox() : nullptr;
    std::shared_ptr<DirectX12Texture> gpuCube;
    if (skybox && skybox->GetRenderProxy())
        gpuCube = skybox->GetRenderProxy()->GetGpuCubemap();

    if (!gpuCube)
    {
        DLOG(LogRenderer, ELogLevel::Warning,
            "DXRenderManager::UpdateIBL: skybox cubemap has no GPU resource yet; falling back to black IBL");
        EnsureIBLFallback();
        return;
    }

    m_iblResources = m_iblBaker.Bake(*m_device, gpuCube);
    m_lastSkyboxTexture = skyboxCubemap;
    m_iblFallbackReady  = false;
}

void DXRenderManager::StageIBLDescriptors(CommandList& commandList)
{
    if (!m_iblResources.irradianceCube || !m_iblResources.specularCube || !m_iblResources.brdfLut)
    {
        DLOG(LogRenderer, ELogLevel::Verbose,
            "DXRenderManager::StageIBLDescriptors skipped: IBL resources not ready (irradiance={}, specular={}, brdfLut={})",
            m_iblResources.irradianceCube != nullptr, m_iblResources.specularCube != nullptr,
            m_iblResources.brdfLut != nullptr);
        return;
    }

    const int32_t rp = static_cast<int32_t>(RootParameterType::IBLTextures);
    commandList.SetShaderResourceView(rp, 0, m_iblResources.irradianceCube,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList.SetShaderResourceView(rp, 1, m_iblResources.specularCube,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList.SetShaderResourceView(rp, 2, m_iblResources.brdfLut,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void DXRenderManager::StageShadowDescriptors(CommandList& commandList)
{
    const bool shadowsOk = m_shadowPass.ShadowResourcesReady();
    auto mapDir = shadowsOk ? m_shadowPass.GetDirectionalAtlasTexture() : nullptr;
    auto mapSpot = shadowsOk ? m_shadowPass.GetSpotAtlasTexture() : nullptr;
    auto cubeAr = shadowsOk ? m_shadowPass.GetPointCubeArrayTexture() : nullptr;

    if (!mapDir || !mapSpot || !cubeAr)
    {
        auto fb2d = DefaultTextures::GetShadowMap2DFallback();
        auto fbCube = DefaultTextures::GetShadowCubeArrayFallback();
        if (!fb2d || !fbCube)
        {
            DLOG(LogRenderer, ELogLevel::Warning,
                "DXRenderManager::StageShadowDescriptors skipped: shadow atlases unavailable and fallback textures missing");
            return;
        }

        mapDir = fb2d;
        mapSpot = fb2d;
        cubeAr = fbCube;
    }

    const int32_t rp = static_cast<int32_t>(RootParameterType::ShadowMaps);
    commandList.SetShaderResourceView(rp, 0, mapDir, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList.SetShaderResourceView(rp, 1, mapSpot, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList.SetShaderResourceView(rp, 2, cubeAr, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    const ShadowSettings& settings = m_shadowPass.GetSettings();
    ShadowCBGPU shadowCb {};
    shadowCb.m_pcssBlockerSamples = (std::clamp)(settings.m_pcssBlockerSamples, 1, 32);
    shadowCb.m_pcssPCFSamples = (std::clamp)(settings.m_pcssPCFSamples, 1, 32);
    shadowCb.m_qualityScalar = (std::max)(0.05f, settings.m_qualityScalar);
    commandList.SetGraphicsDynamicConstantBuffer(static_cast<UINT>(RootParameterType::ShadowCB), shadowCb);
}

void DXRenderManager::RenderFrame()
{
    auto ctx = m_currentContext ? m_currentContext : GetGraphicsContext();
    PostProcessStack* stack = ctx->camera ? ctx->camera->GetPostProcessStack() : nullptr;

    m_frameGraph.Reset();
    BuildFrameGraph(m_pendingSceneDrawCallback, stack);
    m_frameGraph.Compile();
    ExecuteFrameGraph(*ctx, m_pendingSceneDrawCallback);
    m_pendingSceneDrawCallback = nullptr;
    m_frameGraphDirty = false;

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_lastSubmittedFence = directCommandQueue.ExecuteCommandList(m_currentCommandList);
    m_releaseQueue.SetLastSubmittedFence(m_lastSubmittedFence);
    m_device->SetFrameFenceValue(m_lastSubmittedFence);
    m_transientPool.RetireFrame(m_lastSubmittedFence);
    m_currentCommandList = nullptr;
    m_currentContext.reset();
}

RenderResourceReleaseToken DXRenderManager::DeferRenderProxyRelease(std::shared_ptr<RenderProxy> proxy)
{
    return m_releaseQueue.Enqueue(std::move(proxy), m_lastSubmittedFence);
}

void DXRenderManager::Resize(UINT width, UINT height)
{
    DELTA_VERIFY(m_device);
    DLOG_IF(LogRenderer, ELogLevel::Warning, width == 0u || height == 0u,
        "DXRenderManager::Resize received zero dimension ({}x{}); clamping to 1", width, height);

    m_device->Flush();

    m_width = std::max(1u, width);
    m_height = std::max(1u, height);
    m_aspectRatio = static_cast<float>(m_width) / static_cast<float>(m_height);
    m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
    m_renderTarget->Resize(m_width, m_height);
    CreatePingPongTargets(m_width, m_height);
    m_finalPostProcessSRV = {};
    m_finalPostProcessTexture.reset();
    m_frameGraphDirty = true;
}

void DXRenderManager::CreatePingPongTargets(UINT width, UINT height)
{
    auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R16G16B16A16_FLOAT, width, height, 1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    D3D12_CLEAR_VALUE clear{};
    clear.Format = desc.Format;
    clear.Color[0] = 0.0f;
    clear.Color[1] = 0.0f;
    clear.Color[2] = 0.0f;
    clear.Color[3] = 1.0f;

    for (int i = 0; i < 2; ++i)
    {
        m_pingPong[i].texture = m_device->CreateTexture(desc, &clear);
        m_pingPong[i].texture->SetName(i == 0 ? "PostProcess Ping" : "PostProcess Pong");
        m_pingPong[i].rtv = m_pingPong[i].texture->GetRenderTargetView();
        m_pingPong[i].srv = m_pingPong[i].texture->GetShaderResourceView();
    }
}

void DXRenderManager::OnDestroy()
{
    if (m_device)
    {
        m_device->Flush();
        m_releaseQueue.ProcessCompleted();
    }

    // Drop last frame's passes: they hold DXGraphicsContexts whose renderManager
    // shared_ptr points back at us — without this the manager (and device) leak.
    m_frameGraph.Reset();
    m_currentContext.reset();
    m_pendingSceneDrawCallback = nullptr;

    m_transientPool.Clear();
    for (PostProcessPass* pass : m_trackedPasses)
    {
        if (pass)
            pass->Shutdown();
    }
    m_trackedPasses.clear();

    m_iblResources = {};
    m_iblBaker.Shutdown();
    m_shadowPass.Shutdown();

    DefaultTextures::Shutdown();
    CommandList::ClearTextureCache();
}

std::shared_ptr<DXGraphicsContext> DeltaEngine::DXRenderManager::GetGraphicsContext()
{
    if (m_currentContext)
        return m_currentContext;

    auto context = std::make_shared<DXGraphicsContext>();
    context->renderManager = shared_from_this();
    context->device = m_device;
    context->commandList = m_currentCommandList;

    m_currentContext = context;
    return context;
}
