#include "DXRenderManager.h"

#include <iostream>
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <DirectXMath.h>
#include <dxgidebug.h>

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
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Graphics/PostProcess/PostProcessPass.h"
#include "Runtime/Graphics/RenderProxy/CameraRenderProxy.h"
#include "Runtime/Core/DWorld.h"

using namespace Microsoft::WRL;
using namespace DeltaEngine;
using namespace DirectX;

namespace
{
    ComPtr<IDxcBlob> CompilePostProcessVertexShader()
    {
        ComPtr<IDxcUtils> dxcUtils;
        ComPtr<IDxcCompiler3> compiler;
        ComPtr<IDxcIncludeHandler> includeHandler;
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));
        ThrowIfFailed(dxcUtils->CreateDefaultIncludeHandler(&includeHandler));

        const std::wstring shaderPath = IOManager::GetEngineSourceAssetFullPath(L"Shaders/PostProcess_VS.hlsl");
        ComPtr<IDxcBlobEncoding> sourceBlob;
        ThrowIfFailed(dxcUtils->LoadFile(shaderPath.c_str(), nullptr, &sourceBlob));

        BOOL known = FALSE;
        UINT32 encoding = 0;
        ThrowIfFailed(sourceBlob->GetEncoding(&known, &encoding));
        DxcBuffer sourceBuffer{ sourceBlob->GetBufferPointer(), sourceBlob->GetBufferSize(), encoding };

        LPCWSTR args[] = {
            shaderPath.c_str(),
            L"-E", L"main",
            L"-T", L"vs_6_0",
            L"-Zi",
            L"-Fd", L"./",
        };

        ComPtr<IDxcResult> result;
        ThrowIfFailed(compiler->Compile(&sourceBuffer, args, _countof(args), includeHandler.Get(), IID_PPV_ARGS(&result)));

        HRESULT hr = S_OK;
        ThrowIfFailed(result->GetStatus(&hr));
        if (FAILED(hr))
        {
            ComPtr<IDxcBlobEncoding> error;
            result->GetErrorBuffer(&error);
            if (error && error->GetBufferSize() > 0)
            {
                const std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
                std::cerr << "PostProcess_VS compile error: " << errorMessage << std::endl;
            }
            ThrowIfFailed(hr);
        }

        ComPtr<IDxcBlob> blob;
        result->GetResult(&blob);
        return blob;
    }
}

DXRenderManager::DXRenderManager(std::shared_ptr<Device> device, std::shared_ptr<RenderTarget> renderTarget, UINT width, UINT height)
    : m_device(std::move(device)), m_renderTarget(std::move(renderTarget)), m_width(width), m_height(height),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
{
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    LoadPipeline();
    LoadAssets();
}

DXRenderManager::~DXRenderManager() = default;

void DXRenderManager::LoadPipeline()
{
    // Device and RenderTarget are provided by the caller (Editor/Game)
}

void DXRenderManager::LoadAssets()
{
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


    // ==== SRV (t) ====
    // Lights (t0, t1, t2)
    rootParameters[static_cast<UINT>(RootParameterType::PointLights)].InitAsShaderResourceView(0);
    rootParameters[static_cast<UINT>(RootParameterType::SpotLights)].InitAsShaderResourceView(1);
    rootParameters[static_cast<UINT>(RootParameterType::DirectionalLights)].InitAsShaderResourceView(2);

    // Textures (t0+, space1)
    CD3DX12_DESCRIPTOR_RANGE1 ranges[1] {};
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
    rootParameters[static_cast<UINT>(RootParameterType::Texture)].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
    

    // ==== Sampler (s) ====
    // Anisotropic sampler (s0)
    CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);


    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(static_cast<UINT>(RootParameterType::NumRootParameterTypes), rootParameters, 1, &anisotropicSampler, rootSignatureFlags);

    m_rootSignature = m_device->CreateRootSignature(rootSignatureDescription.Desc_1_1);

    m_postProcessVS = CompilePostProcessVertexShader();
}

void DXRenderManager::InitWorldRenderers(DWorld& world)
{
    // Create a color buffer with sRGB for gamma correction.
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
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
    colorTexture->SetName(L"Color Render Target");

    // Create a depth buffer.
    auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat, m_width, m_height, 1, 1, sampleDesc.Count,
        sampleDesc.Quality, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE depthClearValue;
    depthClearValue.Format = depthDesc.Format;
    depthClearValue.DepthStencil = { 1.0f, 0 };

    auto depthTexture = m_device->CreateTexture(depthDesc, &depthClearValue);
    depthTexture->SetName(L"Depth Render Target");

    m_renderTarget->AttachTexture(AttachmentPoint::Color0, colorTexture);
    m_renderTarget->AttachTexture(AttachmentPoint::DepthStencil, depthTexture);

    CreatePingPongTargets(m_width, m_height);

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = directCommandQueue.GetCommandList();
    m_currentCommandList = commandList;

    auto context = GetGraphicsContext();
    world.InitRenderers(context);

    directCommandQueue.ExecuteCommandList(m_currentCommandList);
    m_currentCommandList = nullptr;
    m_currentContext.reset();
}

void DXRenderManager::PrepareFrame()
{
    m_device->ReleaseStaleDescriptors();

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = directCommandQueue.GetCommandList();
    m_currentCommandList = commandList;
    m_currentContext.reset();

    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    commandList->ClearTexture(m_renderTarget->GetTexture(AttachmentPoint::Color0), clearColor);
    commandList->ClearDepthStencilTexture(m_renderTarget->GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);

    commandList->SetViewport(m_viewport);
    commandList->SetScissorRect(m_scissorRect);
    commandList->SetRenderTarget(*m_renderTarget);
    commandList->SetGraphicsRootSignature(m_rootSignature);
}

void DXRenderManager::RenderFrame()
{
    auto ctx = m_currentContext ? m_currentContext : GetGraphicsContext();
    PostProcessStack* stack = ctx->camera ? ctx->camera->postProcessStack : nullptr;
    ExecutePostProcessStack(*ctx, stack, m_width, m_height);

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    directCommandQueue.ExecuteCommandList(m_currentCommandList);
    m_currentCommandList = nullptr;
    m_currentContext.reset();
}

void DXRenderManager::Resize(UINT width, UINT height)
{
    m_width = std::max(1u, width);
    m_height = std::max(1u, height);
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
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
            m_resolvedScene->SetName(L"PostProcess Resolved Scene");
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
        m_pingPong[i].texture->SetName(i == 0 ? L"PostProcess Ping" : L"PostProcess Pong");
        m_pingPong[i].rtv = m_pingPong[i].texture->GetRenderTargetView();
        m_pingPong[i].srv = m_pingPong[i].texture->GetShaderResourceView();
    }
}

void DXRenderManager::OnDestroy()
{
    for (PostProcessPass* pass : m_trackedPasses)
    {
        if (pass)
            pass->Shutdown();
    }
    m_trackedPasses.clear();
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
