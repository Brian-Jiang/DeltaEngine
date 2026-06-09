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

/// Graph node for the forward scene pass: the graph clears + transitions the
/// scene color/depth targets, then this pass binds the render target, root
/// signature and frame descriptors before invoking the scene draw callback.
class DELTAENGINE_API SceneRenderGraphPass final : public RenderGraphPass
{
public:
    using DescriptorStageCallback = std::function<void(CommandList&)>;
    using SceneDrawCallback = std::function<void(const std::shared_ptr<DXGraphicsContext>&)>;

    SceneRenderGraphPass(RenderGraphTextureHandle color,
        RenderGraphTextureHandle depth,
        RenderGraphClearValue colorClear,
        RenderGraphClearValue depthClear,
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
    RenderGraphClearValue m_colorClear;
    RenderGraphClearValue m_depthClear;
    RenderTarget* m_renderTarget = nullptr;
    std::shared_ptr<RootSignature> m_rootSignature;
    CD3DX12_VIEWPORT m_viewport{};
    D3D12_RECT m_scissorRect{};
    DescriptorStageCallback m_stageDescriptors;
    std::shared_ptr<DXGraphicsContext> m_graphicsContext;
    SceneDrawCallback m_drawCallback;
};

DELTA_ENGINE_NS_END
