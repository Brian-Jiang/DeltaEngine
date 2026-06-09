#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
#include "Runtime/Graphics/RenderGraph/SceneShadowReadGraphPass.h"
#include "Runtime/Graphics/RenderGraph/ShadowRenderGraphPass.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
struct ShadowHandles
{
    RenderGraphTextureHandle directional;
    RenderGraphTextureHandle spot;
    RenderGraphTextureHandle point;
};

ShadowHandles ImportShadowAtlases(RenderGraph& graph)
{
    const RenderGraphTextureUsage depthAndShader =
        RenderGraphTextureUsage::DepthAttachment | RenderGraphTextureUsage::ShaderResource;

    ShadowHandles handles;
    handles.directional = graph.ImportTexture("ShadowDirectional", nullptr, depthAndShader);
    handles.spot = graph.ImportTexture("ShadowSpot", nullptr, depthAndShader);
    handles.point = graph.ImportTexture("ShadowPointCubes", nullptr, depthAndShader);
    return handles;
}

std::unique_ptr<ShadowRenderGraphPass> MakeShadowPass(const ShadowHandles& h)
{
    return std::make_unique<ShadowRenderGraphPass>(
        nullptr, nullptr, nullptr, h.directional, h.spot, h.point);
}
} // namespace

TEST(ShadowRenderGraphCompileTests, ShadowThenSceneRead_EmitsDepthWriteThenShaderResourceTransitions)
{
    RenderGraph graph;
    const ShadowHandles h = ImportShadowAtlases(graph);

    graph.AddPass(MakeShadowPass(h));
    graph.AddPass(std::make_unique<SceneShadowReadGraphPass>(h.directional, h.spot, h.point));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 2u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "Shadow");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(1).passIndex).GetName(), "SceneShadowRead");

    const auto& shadow = graph.GetCompiledPass(0).transitions;
    ASSERT_EQ(shadow.size(), 3u);
    EXPECT_EQ(shadow[0].texture, h.directional);
    EXPECT_EQ(shadow[0].stateAfter, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    EXPECT_EQ(shadow[1].texture, h.spot);
    EXPECT_EQ(shadow[1].stateAfter, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    EXPECT_EQ(shadow[2].texture, h.point);
    EXPECT_EQ(shadow[2].stateAfter, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    const auto& sceneRead = graph.GetCompiledPass(1).transitions;
    ASSERT_EQ(sceneRead.size(), 3u);
    EXPECT_EQ(sceneRead[0].texture, h.directional);
    EXPECT_EQ(sceneRead[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(sceneRead[1].texture, h.spot);
    EXPECT_EQ(sceneRead[1].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(sceneRead[2].texture, h.point);
    EXPECT_EQ(sceneRead[2].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

TEST(ShadowRenderGraphCompileTests, ReaderDeclaredFirst_WriterStillOrderedFirst)
{
    RenderGraph graph;
    const ShadowHandles h = ImportShadowAtlases(graph);

    graph.AddPass(std::make_unique<SceneShadowReadGraphPass>(h.directional, h.spot, h.point));
    graph.AddPass(MakeShadowPass(h));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 2u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "Shadow");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(1).passIndex).GetName(), "SceneShadowRead");
}

TEST(ShadowRenderGraphCompileTests, ShadowPassesDoNotAffectUnrelatedResources)
{
    RenderGraph graph;
    const ShadowHandles h = ImportShadowAtlases(graph);

    const RenderGraphTextureHandle sceneColor = graph.ImportTexture("SceneColor", nullptr,
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource);

    graph.AddPass(MakeShadowPass(h));
    graph.AddPass(std::make_unique<SceneShadowReadGraphPass>(h.directional, h.spot, h.point));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 2u);
    for (size_t i = 0; i < graph.GetCompiledPassCount(); ++i)
    {
        for (const auto& transition : graph.GetCompiledPass(i).transitions)
        {
            EXPECT_NE(transition.texture, sceneColor);
        }
    }
}
