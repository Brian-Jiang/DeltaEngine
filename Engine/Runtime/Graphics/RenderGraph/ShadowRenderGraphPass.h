#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"

DELTA_ENGINE_NS_BEGIN

class ShadowPassManager;
class DWorld;
struct DXGraphicsContext;

class DELTAENGINE_API ShadowRenderGraphPass final : public RenderGraphPass
{
public:
    ShadowRenderGraphPass(ShadowPassManager* shadowPass,
        DWorld* world,
        std::shared_ptr<DXGraphicsContext> graphicsContext,
        RenderGraphTextureHandle directionalAtlas,
        RenderGraphTextureHandle spotAtlas,
        RenderGraphTextureHandle pointCubeArray);

    const char* GetName() const override;
    void Setup(RenderGraphBuilder& builder) override;
    void Execute(const RenderGraphContext& context) const override;

private:
    ShadowPassManager* m_shadowPass = nullptr;
    DWorld* m_world = nullptr;
    std::shared_ptr<DXGraphicsContext> m_graphicsContext;
    RenderGraphTextureHandle m_directionalAtlas;
    RenderGraphTextureHandle m_spotAtlas;
    RenderGraphTextureHandle m_pointCubeArray;
};

DELTA_ENGINE_NS_END
