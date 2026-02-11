//#include "SpriteRenderer.h"
//
//#include <d3dx12.h>
//#include <dxcapi.h>
//#include <iostream>
//
//#include "Core/DTexture.h"
//#include "Graphics/DXUtils.h"
//#include "IO/IOManager.h"
//#include "Runtime/Graphics/DirectX/Device.h"
//#include "Runtime/Graphics/DirectX/CommandList.h"
//#include "Runtime/Graphics/DirectX/RootSignature.h"
//
//using namespace DirectX;
//using namespace Microsoft::WRL;
//using namespace DeltaEngine;
//
//SpriteRenderer::SpriteRenderer(): width(0), height(0), vertexBufferView()
//{
//}
//
//SpriteRenderer::~SpriteRenderer()
//{
//}
//
//void SpriteRenderer::Start(float width, float height, const char* texturePath)
//{
//	this->width = width;
//	this->height = height;
//    this->m_texturePath = texturePath;
//}
//
//void DeltaEngine::SpriteRenderer::InitGraphicState(DXGraphicsContext& context) {
//    auto d3d12Device = context.device->GetD3D12Device();
//    auto& srvHeap = context.srvHeap;
//    auto& commandList = context.commandList;
//
//    // ---- Create per-renderer PSO ----
//    {
//        // Compile shaders using DXC.
//        ComPtr<IDxcUtils> dxcUtils;
//        ComPtr<IDxcCompiler3> compiler;
//        ComPtr<IDxcIncludeHandler> includeHandler;
//        ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
//        ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));
//        ThrowIfFailed(dxcUtils->CreateDefaultIncludeHandler(&includeHandler));
//
//        std::wstring shaderPath = IOManager::GetAssetFullPath(L"Shaders/Shaders.hlsl");
//        ComPtr<IDxcBlobEncoding> sourceBlob;
//        ThrowIfFailed(dxcUtils->LoadFile(shaderPath.c_str(), nullptr, &sourceBlob));
//
//        BOOL known;
//        UINT32 encoding;
//        ThrowIfFailed(sourceBlob->GetEncoding(&known, &encoding));
//        DxcBuffer sourceBuffer{ .Ptr = sourceBlob->GetBufferPointer(), .Size = sourceBlob->GetBufferSize(), .Encoding = encoding };
//
//        // Vertex shader
//        ComPtr<IDxcCompilerArgs> arguments;
//        ThrowIfFailed(dxcUtils->BuildArguments(shaderPath.c_str(), L"VSMain", L"vs_6_0", nullptr, 0, nullptr, 0, &arguments));
//        ComPtr<IDxcResult> vsResult;
//        ThrowIfFailed(compiler->Compile(&sourceBuffer, arguments->GetArguments(), arguments->GetCount(), includeHandler.Get(), IID_PPV_ARGS(&vsResult)));
//        HRESULT hr;
//        ThrowIfFailed(vsResult->GetStatus(&hr));
//        if (FAILED(hr)) {
//            ComPtr<IDxcBlobEncoding> error;
//            vsResult->GetErrorBuffer(&error);
//            std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
//            std::cerr << errorMessage << std::endl;
//        }
//        ComPtr<IDxcBlob> vertexShader;
//        vsResult->GetResult(&vertexShader);
//
//        // Pixel shader
//        ThrowIfFailed(dxcUtils->BuildArguments(shaderPath.c_str(), L"PSMain", L"ps_6_0", nullptr, 0, nullptr, 0, &arguments));
//        ComPtr<IDxcResult> psResult;
//        compiler->Compile(&sourceBuffer, arguments->GetArguments(), arguments->GetCount(), includeHandler.Get(), IID_PPV_ARGS(&psResult));
//        psResult->GetStatus(&hr);
//        if (FAILED(hr)) {
//            ComPtr<IDxcBlobEncoding> error;
//            psResult->GetErrorBuffer(&error);
//            std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
//            std::cerr << errorMessage << std::endl;
//        }
//        ComPtr<IDxcBlob> pixelShader;
//        psResult->GetResult(&pixelShader);
//
//        // Input layout (matches Shaders.hlsl expectations)
//        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
//        {
//            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//            // Instance data (per-instance, unique to each instance)
//            { "INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
//            { "INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
//            { "INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
//            { "INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
//            { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
//        };
//
//        D3D12_BLEND_DESC blendDesc = {};
//        blendDesc.AlphaToCoverageEnable = FALSE;
//        blendDesc.IndependentBlendEnable = FALSE;
//        blendDesc.RenderTarget[0].BlendEnable = FALSE;
//        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
//        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
//        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
//        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
//        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
//        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
//        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//
//        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
//        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
//        psoDesc.pRootSignature = context.rootSignature->GetD3D12RootSignature().Get();
//        psoDesc.VS = { static_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
//        psoDesc.PS = { static_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
//        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
//        psoDesc.BlendState = blendDesc;
//        psoDesc.DepthStencilState.DepthEnable = TRUE;
//        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
//        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
//        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
//        psoDesc.DepthStencilState.StencilEnable = FALSE;
//        psoDesc.SampleMask = UINT_MAX;
//        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
//        psoDesc.NumRenderTargets = 1;
//        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
//        psoDesc.SampleDesc.Count = 1;
//        ThrowIfFailed(d3d12Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
//    }
//
//    // ---- Create the vertex buffer ----
//    {
//        SimpleMath::Vector3 position = GetLocalPosition();
//        float x = position.x;
//        float y = position.y;
//
//        Vertex triangleVertices[] =
//        {
//            { { x, y + height, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
//            { { x + width, y, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
//            { { x, y, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f } },
//            { { x, y + height, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f } },
//            { { x + width, y + height, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f } },
//            { { x + width, y, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f } },
//        };
//
//        const UINT vertexBufferSize = sizeof(triangleVertices);
//
//        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
//        auto desc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
//        ThrowIfFailed(d3d12Device->CreateCommittedResource(
//            &heapProps,
//            D3D12_HEAP_FLAG_NONE,
//            &desc,
//            D3D12_RESOURCE_STATE_GENERIC_READ,
//            nullptr,
//            IID_PPV_ARGS(&m_vertexBuffer)));
//
//        UINT8* pVertexDataBegin;
//        CD3DX12_RANGE readRange(0, 0);
//        ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
//        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
//        m_vertexBuffer->Unmap(0, nullptr);
//
//        vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
//        vertexBufferView.StrideInBytes = sizeof(Vertex);
//        vertexBufferView.SizeInBytes = sizeof(triangleVertices);
//    }
//
//    // ---- Create the texture via device (upload + SRV handled internally) ----
//    {
//        m_texture = DTexture::LoadFromFile(std::string(m_texturePath));
//        if (m_texture && !m_texture->GetData().empty())
//            context.device->CreateTextureFromFile(m_texture.get(), *context.commandList);
//    }
//
//    // ---- Per-object constant buffer (root parameter 2: world matrix + color) ----
//    {
//        const UINT objectCbSize = 256; // Aligned for CBV
//        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
//        auto desc = CD3DX12_RESOURCE_DESC::Buffer(objectCbSize);
//        ThrowIfFailed(d3d12Device->CreateCommittedResource(
//            &heapProps,
//            D3D12_HEAP_FLAG_NONE,
//            &desc,
//            D3D12_RESOURCE_STATE_GENERIC_READ,
//            nullptr,
//            IID_PPV_ARGS(&m_objectCb)));
//    }
//}
//
//void DeltaEngine::SpriteRenderer::GatherDrawCalls(DXGraphicsContext& context) {
//    auto& commandList = context.commandList;
//
//    // Per-object: model matrix from SceneComponent, set object CB at root [2].
//    DirectX::XMMATRIX world = GetWorldTransform();
//    struct ObjectData {
//        DirectX::XMFLOAT4X4 worldMatrix;
//        DirectX::XMFLOAT4 color;
//        uint32_t useInstanceMatrix;
//    } obj;
//    DirectX::XMStoreFloat4x4(&obj.worldMatrix, world);
//    obj.color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
//    obj.useInstanceMatrix = 0;
//    void* pObj = nullptr;
//    m_objectCb->Map(0, nullptr, &pObj);
//    memcpy(pObj, &obj, sizeof(obj));
//    m_objectCb->Unmap(0, nullptr);
//
//    commandList->SetPipelineState(m_pipelineState.Get());
//    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//    commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
//    commandList->SetGraphicsRootConstantBufferView(2, m_objectCb->GetGPUVirtualAddress());
//    commandList->SetShaderResourceView(1, m_texture);
//    commandList->Draw(6, 1, 0, 0);
//}
