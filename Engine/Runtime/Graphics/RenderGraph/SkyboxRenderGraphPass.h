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
class DWorld;
class RenderTarget;
class RootSignature;
struct DXGraphicsContext;

class DELTAENGINE_API SkyboxRenderGraphPass final : public RenderGraphPass
{
public:
    using DescriptorStageCallback = std::function<void(CommandList&)>;

    SkyboxRenderGraphPass(RenderGraphTextureHandle color,
        RenderGraphTextureHandle depth,
        RenderTarget* renderTarget,
        std::shared_ptr<RootSignature> rootSignature,
        CD3DX12_VIEWPORT viewport,
        D3D12_RECT scissorRect,
        DescriptorStageCallback stageDescriptors,
        DWorld* world,
        std::shared_ptr<DXGraphicsContext> graphicsContext);

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
    DWorld* m_world = nullptr;
    std::shared_ptr<DXGraphicsContext> m_graphicsContext;
};

DELTA_ENGINE_NS_END
