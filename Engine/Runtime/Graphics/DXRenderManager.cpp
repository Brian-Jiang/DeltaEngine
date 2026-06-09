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

using namespace Microsoft::WRL;
using namespace DeltaEngine;
using namespace DirectX;

DXRenderManager::DXRenderManager(std::shared_ptr<Device> device, std::shared_ptr<RenderTarget> renderTarget, UINT width, UINT height)
    : m_device(std::move(device)), m_renderTarget(std::move(renderTarget)), m_width(width), m_height(height),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
{
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

    m_releaseQueue.SetFenceCompleteChecker([this](uint64_t fenceValue)
    {
        return m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).IsFenceComplete(fenceValue);
    });
    GetRenderResourceReleaseService().RegisterQueue(&m_releaseQueue);

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

    m_iblBaker.Initialize(*m_device);
    m_shadowPass.Initialize(*m_device);
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
    DXGI_SAMPLE_DESC sampleDesc = m_device->GetMultisampleQualityLevels(backBufferFormat);

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

void DXRenderManager::PrepareFrame()
{
    m_releaseQueue.ProcessCompleted();
    m_device->ReleaseStaleDescriptors();

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
    {
        auto ctx = GetGraphicsContext();
        ctx->activeRenderCamera = m_pendingActiveRenderCamera;
        m_pendingActiveRenderCamera.reset();
        if (m_currentWorld)
        {
            m_currentWorld->PreGatherDrawCalls(ctx);
            m_shadowPass.Render(ctx, *m_currentWorld);
        }
    }

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearTexture(m_renderTarget->GetTexture(AttachmentPoint::Color0), clearColor);
    commandList->ClearDepthStencilTexture(m_renderTarget->GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);

    commandList->SetViewport(m_viewport);
    commandList->SetScissorRect(m_scissorRect);
    commandList->SetRenderTarget(*m_renderTarget);
    commandList->SetGraphicsRootSignature(m_rootSignature);

    StageIBLDescriptors(*commandList);
    StageShadowDescriptors(*commandList);
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
    ExecutePostProcessStack(*ctx, stack, m_width, m_height);

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_lastSubmittedFence = directCommandQueue.ExecuteCommandList(m_currentCommandList);
    m_releaseQueue.SetLastSubmittedFence(m_lastSubmittedFence);
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
    m_resolvedScene.reset();
    m_finalPostProcessSRV = {};
    m_finalPostProcessTexture.reset();
}

void DXRenderManager::ExecutePostProcessStack(DXGraphicsContext& ctx, PostProcessStack* stack, UINT width, UINT height)
{
    auto sceneTex = m_renderTarget->GetTexture(AttachmentPoint::Color0);
    D3D12_CPU_DESCRIPTOR_HANDLE sceneSRV = sceneTex->GetShaderResourceView();

    if (!stack || stack->GetPassCount() == 0)
    {
        m_finalPostProcessSRV = sceneSRV;
        m_hasPostProcessedOutput = false;
        m_finalPostProcessTexture.reset();
        return;
    }

    auto& cl = *m_currentCommandList;

    const auto sceneDesc = sceneTex->GetD3D12ResourceDesc();
    if (sceneDesc.SampleDesc.Count > 1)
    {
        if (!m_resolvedScene ||
            m_resolvedScene->GetD3D12ResourceDesc().Width != sceneDesc.Width ||
            m_resolvedScene->GetD3D12ResourceDesc().Height != sceneDesc.Height ||
            m_resolvedScene->GetD3D12ResourceDesc().Format != sceneDesc.Format)
        {
            auto resolvedDesc = CD3DX12_RESOURCE_DESC::Tex2D(
                sceneDesc.Format, sceneDesc.Width, static_cast<UINT>(sceneDesc.Height),
                1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE);
            m_resolvedScene = m_device->CreateTexture(resolvedDesc, nullptr);
            m_resolvedScene->SetName("PostProcess Resolved Scene");
        }

        cl.ResolveSubresource(m_resolvedScene, sceneTex);
        cl.TransitionBarrier(m_resolvedScene, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        sceneSRV = m_resolvedScene->GetShaderResourceView();
    }
    else
    {
        cl.TransitionBarrier(sceneTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE readSRV = sceneSRV;
    int writeIdx = 0;

    const int passCount = stack->GetPassCount();
    PIXBeginEvent(cl.GetD3D12CommandList().Get(), PIX_COLOR_DEFAULT, L"PostProcess");
    for (int i = 0; i < passCount; ++i)
    {
        PostProcessPass* pass = stack->GetPass(i);
        if (!pass)
            continue;

        m_trackedPasses.insert(pass);

        auto& dst = m_pingPong[writeIdx];
        cl.TransitionBarrier(dst.texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
        cl.FlushResourceBarriers();

        const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        cl.GetD3D12CommandList()->ClearRenderTargetView(dst.rtv, black, 0, nullptr);
        cl.GetD3D12CommandList()->OMSetRenderTargets(1, &dst.rtv, FALSE, nullptr);

        pass->Execute(ctx, readSRV, dst.rtv, width, height);

        cl.TransitionBarrier(dst.texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        readSRV = dst.srv;
        writeIdx = 1 - writeIdx;
    }
    PIXEndEvent(cl.GetD3D12CommandList().Get());

    m_finalPostProcessSRV = readSRV;
    m_hasPostProcessedOutput = true;
    m_finalPostProcessTexture = m_pingPong[1 - writeIdx].texture;
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
