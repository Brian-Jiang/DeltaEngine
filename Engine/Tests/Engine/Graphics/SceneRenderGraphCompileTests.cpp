#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
#include "Runtime/Graphics/RenderGraph/PostProcessFinalizeGraphPass.h"
#include "Runtime/Graphics/RenderGraph/PostProcessRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/SceneRenderGraphPass.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
constexpr float kClearR = 0.0f;
constexpr float kClearG = 0.2f;
constexpr float kClearB = 0.4f;
constexpr float kClearA = 1.0f;

void AddScenePass(RenderGraph& graph, RenderGraphTextureHandle color, RenderGraphTextureHandle depth)
{
    graph.AddPass(std::make_unique<SceneRenderGraphPass>(
        color, depth,
        RenderGraphClearValue::Color4(kClearR, kClearG, kClearB, kClearA),
        RenderGraphClearValue::DepthStencil(1.0f),
        nullptr, nullptr, CD3DX12_VIEWPORT(0.0f, 0.0f, 1920.0f, 1080.0f), CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX),
        nullptr, nullptr, nullptr));
}

D3D12_CPU_DESCRIPTOR_HANDLE MakeDummyHandle(UINT index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    handle.ptr = static_cast<SIZE_T>(index);
    return handle;
}
} // namespace

TEST(SceneRenderGraphCompileTests, ScenePass_EmitsColorDepthTransitionsAndClears)
{
    RenderGraph graph;
    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr,
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);

    AddScenePass(graph, color, depth);
    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 1u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "Scene");

    const auto& transitions = graph.GetCompiledPass(0).transitions;
    ASSERT_EQ(transitions.size(), 2u);
    EXPECT_EQ(transitions[0].texture, color);
    EXPECT_EQ(transitions[0].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);
    EXPECT_EQ(transitions[1].texture, depth);
    EXPECT_EQ(transitions[1].stateAfter, D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

TEST(SceneRenderGraphCompileTests, ScenePass_ClearOpMetadataSurfacesInCompiledPass)
{
    RenderGraph graph;
    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr,
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);

    AddScenePass(graph, color, depth);
    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 1u);
    const auto& clears = graph.GetCompiledPass(0).clears;
    ASSERT_EQ(clears.size(), 2u);

    EXPECT_EQ(clears[0].texture, color);
    EXPECT_EQ(clears[0].value.type, RenderGraphClearValue::Type::Color);
    EXPECT_FLOAT_EQ(clears[0].value.color[0], kClearR);
    EXPECT_FLOAT_EQ(clears[0].value.color[1], kClearG);
    EXPECT_FLOAT_EQ(clears[0].value.color[2], kClearB);
    EXPECT_FLOAT_EQ(clears[0].value.color[3], kClearA);

    EXPECT_EQ(clears[1].texture, depth);
    EXPECT_EQ(clears[1].value.type, RenderGraphClearValue::Type::DepthStencil);
    EXPECT_FLOAT_EQ(clears[1].value.depth, 1.0f);
    EXPECT_EQ(clears[1].value.stencil, 0);
}

TEST(SceneRenderGraphCompileTests, SceneWriteThenPostProcessRead_OrdersAndTransitionsToShaderResource)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);
    const RenderGraphTextureHandle ping = graph.ImportTexture("Ping", nullptr, colorAndShader);

    AddScenePass(graph, color, depth);
    graph.AddPass(std::make_unique<PostProcessRenderGraphPass>(
        nullptr, color, ping, MakeDummyHandle(0), MakeDummyHandle(1), 1920, 1080));
    graph.AddPass(std::make_unique<PostProcessFinalizeGraphPass>(ping));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 3u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "Scene");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(1).passIndex).GetName(), "PostProcess");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(2).passIndex).GetName(), "PostProcessFinalize");

    const auto& ppTransitions = graph.GetCompiledPass(1).transitions;
    ASSERT_EQ(ppTransitions.size(), 2u);
    EXPECT_EQ(ppTransitions[0].texture, color);
    EXPECT_EQ(ppTransitions[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(ppTransitions[1].texture, ping);
    EXPECT_EQ(ppTransitions[1].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

TEST(SceneRenderGraphCompileTests, PostProcessDeclaredBeforeScene_TopoSortRunsSceneFirst)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);
    const RenderGraphTextureHandle ping = graph.ImportTexture("Ping", nullptr, colorAndShader);

    graph.AddPass(std::make_unique<PostProcessRenderGraphPass>(
        nullptr, color, ping, MakeDummyHandle(0), MakeDummyHandle(1), 1920, 1080));
    AddScenePass(graph, color, depth);

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 2u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "Scene");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(1).passIndex).GetName(), "PostProcess");
}
