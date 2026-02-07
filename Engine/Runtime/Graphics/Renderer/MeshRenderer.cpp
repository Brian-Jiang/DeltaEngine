#include "MeshRenderer.h"

#include <d3dx12.h>
#include <dxcapi.h>
#include <iostream>

#include "Graphics/Texture.h"
#include "Graphics/DXUtils.h"
#include "IO/IOManager.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/CommandList.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace DeltaEngine;

MeshRenderer::MeshRenderer() : meshCount(0), loadedTextureCount(0)
{
}

MeshRenderer::~MeshRenderer()
{
}

void MeshRenderer::Start(const std::vector<Mesh*>& meshes, const std::vector<XMMATRIX>& meshTransforms)
{
	this->meshes = meshes;
	this->meshTransforms = meshTransforms;
}

void DeltaEngine::MeshRenderer::InitGraphicState(DXGraphicsContext& context) {
    auto d3d12Device = context.device->GetD3D12Device();

    // ---- Compile shaders and create PSO ----
    {
        ComPtr<IDxcUtils> dxcUtils;
        ComPtr<IDxcCompiler3> compiler;
        ComPtr<IDxcIncludeHandler> includeHandler;
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
        ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils)));
        ThrowIfFailed(dxcUtils->CreateDefaultIncludeHandler(&includeHandler));

        std::wstring shaderPath = IOManager::GetAssetFullPath(L"Shaders/Shaders.hlsl");
        ComPtr<IDxcBlobEncoding> sourceBlob;
        ThrowIfFailed(dxcUtils->LoadFile(shaderPath.c_str(), nullptr, &sourceBlob));

        BOOL known;
        UINT32 encoding;
        ThrowIfFailed(sourceBlob->GetEncoding(&known, &encoding));
        DxcBuffer sourceBuffer{ .Ptr = sourceBlob->GetBufferPointer(), .Size = sourceBlob->GetBufferSize(), .Encoding = encoding };

        // Vertex shader
        ComPtr<IDxcCompilerArgs> arguments;
        ThrowIfFailed(dxcUtils->BuildArguments(shaderPath.c_str(), L"VSMain", L"vs_6_0", nullptr, 0, nullptr, 0, &arguments));
        ComPtr<IDxcResult> vsResult;
        ThrowIfFailed(compiler->Compile(&sourceBuffer, arguments->GetArguments(), arguments->GetCount(), includeHandler.Get(), IID_PPV_ARGS(&vsResult)));
        HRESULT hr;
        ThrowIfFailed(vsResult->GetStatus(&hr));
        if (FAILED(hr)) {
            ComPtr<IDxcBlobEncoding> error;
            vsResult->GetErrorBuffer(&error);
            std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
            std::cerr << errorMessage << std::endl;
        }
        ComPtr<IDxcBlob> vertexShader;
        vsResult->GetResult(&vertexShader);

        // Pixel shader
        ThrowIfFailed(dxcUtils->BuildArguments(shaderPath.c_str(), L"PSMain", L"ps_6_0", nullptr, 0, nullptr, 0, &arguments));
        ComPtr<IDxcResult> psResult;
        compiler->Compile(&sourceBuffer, arguments->GetArguments(), arguments->GetCount(), includeHandler.Get(), IID_PPV_ARGS(&psResult));
        psResult->GetStatus(&hr);
        if (FAILED(hr)) {
            ComPtr<IDxcBlobEncoding> error;
            psResult->GetErrorBuffer(&error);
            std::string errorMessage(static_cast<const char*>(error->GetBufferPointer()), error->GetBufferSize());
            std::cerr << errorMessage << std::endl;
        }
        ComPtr<IDxcBlob> pixelShader;
        psResult->GetResult(&pixelShader);

        // Input layout matching Shaders.hlsl
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

            // Instance data (per-instance, unique to each instance)
            { "INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
            { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        };

        D3D12_BLEND_DESC blendDesc = {};
        blendDesc.AlphaToCoverageEnable = FALSE;
        blendDesc.IndependentBlendEnable = FALSE;
        blendDesc.RenderTarget[0].BlendEnable = FALSE;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = context.rootSignature.Get();
        psoDesc.VS = { static_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
        psoDesc.PS = { static_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = blendDesc;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;
        ThrowIfFailed(d3d12Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    // ---- Create vertex/index buffers and load textures for all meshes ----
    for (size_t i = 0; i < meshes.size(); ++i) {
        auto mesh = meshes[i];
        auto meshTransform = meshTransforms[i];
        AddMesh(mesh, meshTransform, context);
    }
}

void MeshRenderer::AddMesh(const Mesh* mesh, const XMMATRIX meshTransform, const DXGraphicsContext& context)
{
    auto& uploadBuffer = EngineMain::instance->dxRenderManager->GetUploadBuffer();

    // Create the vertex buffer.
    {
        const UINT vertexBufferSize = static_cast<UINT>(mesh->vertices.size() * sizeof(Vertex));
        auto addrPair = uploadBuffer.Allocate(vertexBufferSize, sizeof(Vertex));
        memcpy(addrPair.CPU, mesh->vertices.data(), vertexBufferSize);
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView{
            addrPair.GPU,
            vertexBufferSize,
            sizeof(Vertex)
        };

        vertexBufferViews.push_back(vertexBufferView);
    }

    // Create the index buffer.
    {
        const UINT indexBufferSize = static_cast<UINT>(mesh->indices.size() * sizeof(unsigned int));
        auto addrPair = uploadBuffer.Allocate(indexBufferSize, sizeof(unsigned int));
        memcpy(addrPair.CPU, mesh->indices.data(), indexBufferSize);
        D3D12_INDEX_BUFFER_VIEW indexBufferView{
            addrPair.GPU,
            indexBufferSize,
            DXGI_FORMAT_R32_UINT
        };

        indexBufferViews.push_back(indexBufferView);
    }

    // Create model matrix
    {
        const UINT constantBufferSize = sizeof(DirectX::XMFLOAT4X4);
        auto addrPair = uploadBuffer.Allocate(constantBufferSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        DirectX::XMStoreFloat4x4(static_cast<DirectX::XMFLOAT4X4*>(addrPair.CPU), meshTransform);
    }

    for (size_t i = 0; i < mesh->textures.size(); ++i) {
        auto texture = mesh->textures[i];
        LoadTexture(texture, context, loadedTextureCount++);
    }

    ++meshCount;
}

void MeshRenderer::LoadTexture(const Texture* texture, const DXGraphicsContext& context, UINT descriptorIndex)
{
    auto d3d12Device = context.device->GetD3D12Device();
    auto& commandList = context.commandList;

    auto textureHeight = texture->GetHeight();
    auto textureWidth = texture->GetWidth();

    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = textureWidth;
    textureDesc.Height = textureHeight;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    auto hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ComPtr<ID3D12Resource> textureResource;
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
        &hp,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&textureResource)));

    UINT64 rowPitch = textureWidth * 4;
    UINT64 alignedRowPitch = (rowPitch + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    UINT64 textureSize = alignedRowPitch * textureHeight;

    hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadHeapDesc = CD3DX12_RESOURCE_DESC::Buffer(textureSize);
    ComPtr<ID3D12Resource> textureUploadHeap;
    ThrowIfFailed(d3d12Device->CreateCommittedResource(
        &hp,
        D3D12_HEAP_FLAG_NONE,
        &uploadHeapDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&textureUploadHeap)));

    auto rawData = texture->GetData().data();

    D3D12_SUBRESOURCE_FOOTPRINT pitchedDesc = {};
    pitchedDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    pitchedDesc.Width = textureWidth;
    pitchedDesc.Height = textureHeight;
    pitchedDesc.Depth = 1;
    pitchedDesc.RowPitch = static_cast<UINT>(alignedRowPitch);

    UINT8* pData;
    textureUploadHeap->Map(0, nullptr, reinterpret_cast<void**>(&pData));
    for (UINT y = 0; y < textureHeight; y++) {
        memcpy(pData + y * alignedRowPitch, rawData + y * rowPitch, rowPitch);
    }
    textureUploadHeap->Unmap(0, nullptr);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTexture2D = { 0 };
    placedTexture2D.Offset = 0;
    placedTexture2D.Footprint = pitchedDesc;
    
    D3D12_TEXTURE_COPY_LOCATION dst = CD3DX12_TEXTURE_COPY_LOCATION(textureResource.Get(), 0);
    D3D12_TEXTURE_COPY_LOCATION src = CD3DX12_TEXTURE_COPY_LOCATION(textureUploadHeap.Get(), placedTexture2D);
    commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

    auto rb = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &rb);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(context.srvHeap->GetCPUDescriptorHandleForHeapStart(), descriptorIndex, context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    d3d12Device->CreateShaderResourceView(textureResource.Get(), &srvDesc, srvHandle);

    this->textureResources.push_back(textureResource);
	this->textureUploadResources.push_back(textureUploadHeap);
}

void DeltaEngine::MeshRenderer::GatherDrawCalls(DXGraphicsContext& context)
{
    auto& commandList = context.commandList;

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    for (int i = 0; i < meshCount; ++i) {
        commandList->IASetVertexBuffers(0, 1, &vertexBufferViews[i]);
        commandList->IASetIndexBuffer(&indexBufferViews[i]);

        if (i < loadedTextureCount) {
            commandList->SetGraphicsRootDescriptorTable(1, CD3DX12_GPU_DESCRIPTOR_HANDLE(context.srvHeap->GetGPUDescriptorHandleForHeapStart(), i, context.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)));
        }

        commandList->DrawIndexedInstanced(static_cast<uint32_t>(this->meshes[i]->indices.size()), 1, 0, 0, 0);
    }
}
