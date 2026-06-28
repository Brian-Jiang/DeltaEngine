#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include <d3dx12.h>
#include <functional>
#include <memory>

#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"

DELTA_ENGINE_NS_BEGIN

class CommandList;
class RenderTarget;
class RootSignature;
struct DXGraphicsContext;

/// Graph node for the forward transparent pass: loads existing scene color/depth,
/// binds the render target, stages frame descriptors, then invokes the transparent
/// draw callback (sorted back-to-front, alpha blend, depth write off).
class DELTAENGINE_API TransparentRenderGraphPass final : public RenderGraphPass
{
public:
    using DescriptorStageCallback = std::function<void(CommandList&)>;
    using SceneDrawCallback = std::function<void(const std::shared_ptr<DXGraphicsContext>&)>;

    TransparentRenderGraphPass(RenderGraphTextureHandle color,
        RenderGraphTextureHandle depth,
        RenderTarget* renderTarget,
        std::shared_ptr<RootSignature> rootSignature,
        CD3DX12_VIEWPORT viewport,
        D3D12_RECT scissorRect,
        DescriptorStageCallback stageDescriptors,
        std::shared_ptr<DXGraphicsContext> graphicsContext,
        SceneDrawCallback drawCallback);

    const char* GetName() const override;
    void Setup(RenderGraphBuilder& builder) override;
    void Execute(const RenderGraphContext& context) const override;

private:
    RenderGraphTextureHandle m_color;
    RenderGraphTextureHandle m_depth;
    RenderTarget* m_renderTarget = nullptr;
    std::shared_ptr<RootSignature> m_rootSignature;
    CD3DX12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};
    DescriptorStageCallback m_stageDescriptors;
    std::shared_ptr<DXGraphicsContext> m_graphicsContext;
    SceneDrawCallback m_drawCallback;
};

DELTA_ENGINE_NS_END
