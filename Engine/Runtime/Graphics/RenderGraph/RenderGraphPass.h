#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class RenderGraphBuilder;
struct RenderGraphContext;

class RenderGraphPass
{
public:
    virtual ~RenderGraphPass() = default;

    virtual const char* GetName() const = 0;
    virtual void Setup(RenderGraphBuilder& builder) = 0;
    virtual void Execute(const RenderGraphContext& context) const = 0;
};

DELTA_ENGINE_NS_END
