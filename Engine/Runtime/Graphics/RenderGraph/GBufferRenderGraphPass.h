#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <functional>
#include <memory>

#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"

DELTA_ENGINE_NS_BEGIN

class RootSignature;
struct DXGraphicsContext;

class DELTAENGINE_API GBufferRenderGraphPass final : public RenderGraphPass
{
public:
    using SceneDrawCallback = std::function<void(const std::shared_ptr<DXGraphicsContext>&)>;

    GBufferRenderGraphPass(RenderGraphTextureHandle albedo,
        RenderGraphTextureHandle normal,
        RenderGraphTextureHandle material,
        RenderGraphTextureHandle emissive,
        RenderGraphTextureHandle depth,
        D3D12_CPU_DESCRIPTOR_HANDLE albedoRtv,
        D3D12_CPU_DESCRIPTOR_HANDLE normalRtv,
        D3D12_CPU_DESCRIPTOR_HANDLE materialRtv,
        D3D12_CPU_DESCRIPTOR_HANDLE emissiveRtv,
        D3D12_CPU_DESCRIPTOR_HANDLE depthDsv,
        std::shared_ptr<RootSignature> rootSignature,
        CD3DX12_VIEWPORT viewport,
        D3D12_RECT scissorRect,
        std::shared_ptr<DXGraphicsContext> graphicsContext,
        SceneDrawCallback drawCallback);

    const char* GetName() const override;
    void Setup(RenderGraphBuilder& builder) override;
    void Execute(const RenderGraphContext& context) const override;

private:
    RenderGraphTextureHandle m_albedo;
    RenderGraphTextureHandle m_normal;
    RenderGraphTextureHandle m_material;
    RenderGraphTextureHandle m_emissive;
    RenderGraphTextureHandle m_depth;
    D3D12_CPU_DESCRIPTOR_HANDLE m_albedoRtv{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_normalRtv{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_materialRtv{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_emissiveRtv{};
    D3D12_CPU_DESCRIPTOR_HANDLE m_depthDsv{};
    std::shared_ptr<RootSignature> m_rootSignature;
    CD3DX12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};
    std::shared_ptr<DXGraphicsContext> m_graphicsContext;
    SceneDrawCallback m_drawCallback;
};

DELTA_ENGINE_NS_END
