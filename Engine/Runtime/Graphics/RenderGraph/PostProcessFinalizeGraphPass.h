#pragma once

#include "EngineIncludes.h"

#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphResourceHandle.h"

DELTA_ENGINE_NS_BEGIN

class DELTAENGINE_API PostProcessFinalizeGraphPass final : public RenderGraphPass
{
public:
    explicit PostProcessFinalizeGraphPass(RenderGraphTextureHandle output);

    const char* GetName() const override;
    void Setup(RenderGraphBuilder& builder) override;
    void Execute(const RenderGraphContext& context) const override;

private:
    RenderGraphTextureHandle m_output;
};

DELTA_ENGINE_NS_END
