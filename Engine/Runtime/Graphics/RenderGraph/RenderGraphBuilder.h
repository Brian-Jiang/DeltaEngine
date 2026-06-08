#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class RenderGraph;

class RenderGraphBuilder
{
public:
    explicit RenderGraphBuilder(RenderGraph& graph) : m_graph(graph) {}

    RenderGraph& GetGraph() { return m_graph; }
    const RenderGraph& GetGraph() const { return m_graph; }

private:
    RenderGraph& m_graph;
};

DELTA_ENGINE_NS_END
