#include "DXRenderManager.h"

#include <fstream>
#include <iostream>
#include <d3dcompiler.h>
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
#include "Runtime/Core/DWorld.h"

using namespace Microsoft::WRL;
using namespace DeltaEngine;
using namespace DirectX;

DXRenderManager::DXRenderManager(std::shared_ptr<Device> device, std::shared_ptr<RenderTarget> renderTarget, UINT width, UINT height)
    : m_device(std::move(device)), m_renderTarget(std::move(renderTarget)), m_width(width), m_height(height),
    m_viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
    m_scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
{
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    LoadPipeline();
    LoadAssets();
}

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

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = directCommandQueue.GetCommandList();
    m_currentCommandList = commandList;

    auto context = GetGraphicsContext();
    world.InitRenderers(context);

    directCommandQueue.ExecuteCommandList(m_currentCommandList);
    m_currentCommandList = nullptr;
}

void DXRenderManager::PrepareFrame()
{
    m_device->ReleaseStaleDescriptors();

    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = directCommandQueue.GetCommandList();
    m_currentCommandList = commandList;

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
    CommandQueue& directCommandQueue = m_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    directCommandQueue.ExecuteCommandList(m_currentCommandList);
    m_currentCommandList = nullptr;
}

void DXRenderManager::Resize(UINT width, UINT height)
{
    m_width = std::max(1u, width);
    m_height = std::max(1u, height);
    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height));
    m_renderTarget->Resize(m_width, m_height);
}

void DXRenderManager::OnDestroy()
{

}

std::shared_ptr<DXGraphicsContext> DeltaEngine::DXRenderManager::GetGraphicsContext()
{
    auto context = std::make_shared<DXGraphicsContext>();
    context->renderManager = shared_from_this();
    context->device = m_device;
    context->commandList = m_currentCommandList;

    return context;
}
