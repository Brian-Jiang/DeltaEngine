#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
#include "Runtime/Graphics/RenderGraph/PostProcessFinalizeGraphPass.h"
#include "Runtime/Graphics/RenderGraph/PostProcessRenderGraphPass.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
D3D12_CPU_DESCRIPTOR_HANDLE MakeDummyHandle(UINT index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    handle.ptr = static_cast<SIZE_T>(index);
    return handle;
}

void AddPostProcessPass(RenderGraph& graph,
    RenderGraphTextureHandle input,
    RenderGraphTextureHandle output,
    UINT inputSrvIndex,
    UINT outputRtvIndex)
{
    graph.AddPass(std::make_unique<PostProcessRenderGraphPass>(
        nullptr, input, output, MakeDummyHandle(inputSrvIndex), MakeDummyHandle(outputRtvIndex), 1920, 1080));
}
} // namespace

TEST(PostProcessRenderGraphCompileTests, TwoPassPingPong_OrdersAndEmitsTransitions)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle sceneColor =
        graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle ping =
        graph.ImportTexture("Ping", nullptr, colorAndShader);
    const RenderGraphTextureHandle pong =
        graph.ImportTexture("Pong", nullptr, colorAndShader);

    AddPostProcessPass(graph, sceneColor, ping, 0, 1);
    AddPostProcessPass(graph, ping, pong, 1, 2);
    graph.AddPass(std::make_unique<PostProcessFinalizeGraphPass>(pong));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 3u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "PostProcess");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(1).passIndex).GetName(), "PostProcess");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(2).passIndex).GetName(), "PostProcessFinalize");

    const auto& pass0 = graph.GetCompiledPass(0).transitions;
    ASSERT_EQ(pass0.size(), 2u);
    EXPECT_EQ(pass0[0].texture, sceneColor);
    EXPECT_EQ(pass0[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(pass0[1].texture, ping);
    EXPECT_EQ(pass0[1].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& pass1 = graph.GetCompiledPass(1).transitions;
    ASSERT_EQ(pass1.size(), 2u);
    EXPECT_EQ(pass1[0].texture, ping);
    EXPECT_EQ(pass1[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(pass1[1].texture, pong);
    EXPECT_EQ(pass1[1].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& finalize = graph.GetCompiledPass(2).transitions;
    ASSERT_EQ(finalize.size(), 1u);
    EXPECT_EQ(finalize[0].texture, pong);
    EXPECT_EQ(finalize[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

TEST(PostProcessRenderGraphCompileTests, ThreePassAlternation_PreservesDeclarationOrder)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle sceneColor =
        graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle ping =
        graph.ImportTexture("Ping", nullptr, colorAndShader);
    const RenderGraphTextureHandle pong =
        graph.ImportTexture("Pong", nullptr, colorAndShader);

    AddPostProcessPass(graph, sceneColor, ping, 0, 1);
    AddPostProcessPass(graph, ping, pong, 1, 2);
    AddPostProcessPass(graph, pong, ping, 2, 1);
    graph.AddPass(std::make_unique<PostProcessFinalizeGraphPass>(ping));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 4u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "PostProcess");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(1).passIndex).GetName(), "PostProcess");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(2).passIndex).GetName(), "PostProcess");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(3).passIndex).GetName(), "PostProcessFinalize");

    const auto& pass2 = graph.GetCompiledPass(2).transitions;
    ASSERT_EQ(pass2.size(), 2u);
    EXPECT_EQ(pass2[0].texture, pong);
    EXPECT_EQ(pass2[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(pass2[1].texture, ping);
    EXPECT_EQ(pass2[1].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const auto& finalize = graph.GetCompiledPass(3).transitions;
    ASSERT_EQ(finalize.size(), 1u);
    EXPECT_EQ(finalize[0].texture, ping);
    EXPECT_EQ(finalize[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

TEST(PostProcessRenderGraphCompileTests, FinalizePass_TransitionsLastOutputToShaderResource)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle sceneColor =
        graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle ping =
        graph.ImportTexture("Ping", nullptr, colorAndShader);

    AddPostProcessPass(graph, sceneColor, ping, 0, 1);
    graph.AddPass(std::make_unique<PostProcessFinalizeGraphPass>(ping));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 2u);

    const auto& finalize = graph.GetCompiledPass(1).transitions;
    ASSERT_EQ(finalize.size(), 1u);
    EXPECT_EQ(finalize[0].texture, ping);
    EXPECT_EQ(finalize[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}
