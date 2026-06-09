#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
#include "Runtime/Graphics/RenderGraph/MsaaResolveGraphPass.h"
#include "Runtime/Graphics/RenderGraph/PostProcessFinalizeGraphPass.h"
#include "Runtime/Graphics/RenderGraph/PostProcessRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/SceneRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/SceneShadowReadGraphPass.h"
#include "Runtime/Graphics/RenderGraph/ShadowRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/SkyboxRenderGraphPass.h"

#include <cstring>

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
constexpr float kClearR = 0.0f;
constexpr float kClearG = 0.2f;
constexpr float kClearB = 0.4f;
constexpr float kClearA = 1.0f;

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

void AddScenePass(RenderGraph& graph, RenderGraphTextureHandle color, RenderGraphTextureHandle depth)
{
    graph.AddPass(std::make_unique<SceneRenderGraphPass>(
        color, depth,
        RenderGraphClearValue::Color4(kClearR, kClearG, kClearB, kClearA),
        RenderGraphClearValue::DepthStencil(1.0f),
        nullptr, nullptr, CD3DX12_VIEWPORT(0.0f, 0.0f, 1920.0f, 1080.0f), CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX),
        nullptr, nullptr, nullptr));
}

void AddSkyboxPass(RenderGraph& graph, RenderGraphTextureHandle color, RenderGraphTextureHandle depth)
{
    graph.AddPass(std::make_unique<SkyboxRenderGraphPass>(
        color, depth, nullptr, nullptr, CD3DX12_VIEWPORT(0.0f, 0.0f, 1920.0f, 1080.0f), CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX),
        nullptr, nullptr, nullptr));
}

D3D12_CPU_DESCRIPTOR_HANDLE MakeDummyHandle(UINT index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    handle.ptr = static_cast<SIZE_T>(index);
    return handle;
}

void AddPostProcessPass(RenderGraph& graph, RenderGraphTextureHandle input, RenderGraphTextureHandle output,
    UINT inputSrvIndex, UINT outputRtvIndex)
{
    graph.AddPass(std::make_unique<PostProcessRenderGraphPass>(
        nullptr, input, output, MakeDummyHandle(inputSrvIndex), MakeDummyHandle(outputRtvIndex), 1920, 1080));
}

size_t FindCompiledPassIndex(const RenderGraph& graph, const char* passName)
{
    for (size_t i = 0; i < graph.GetCompiledPassCount(); ++i)
    {
        if (std::strcmp(graph.GetPass(graph.GetCompiledPass(i).passIndex).GetName(), passName) == 0)
            return i;
    }

    return graph.GetCompiledPassCount();
}
} // namespace

TEST(FullFrameRenderGraphCompileTests, FullChain_Order)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);
    const RenderGraphTextureHandle ping = graph.ImportTexture("Ping", nullptr, colorAndShader);
    const ShadowHandles shadow = ImportShadowAtlases(graph);

    graph.AddPass(std::make_unique<ShadowRenderGraphPass>(
        nullptr, nullptr, nullptr, shadow.directional, shadow.spot, shadow.point));
    graph.AddPass(std::make_unique<SceneShadowReadGraphPass>(shadow.directional, shadow.spot, shadow.point));
    AddScenePass(graph, color, depth);
    AddSkyboxPass(graph, color, depth);
    AddPostProcessPass(graph, color, ping, 0, 1);
    graph.AddPass(std::make_unique<PostProcessFinalizeGraphPass>(ping));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 6u);
    const size_t shadowIdx = FindCompiledPassIndex(graph, "Shadow");
    const size_t shadowReadIdx = FindCompiledPassIndex(graph, "SceneShadowRead");
    const size_t skyboxIdx = FindCompiledPassIndex(graph, "Skybox");
    const size_t postIdx = FindCompiledPassIndex(graph, "PostProcess");
    const size_t finalizeIdx = FindCompiledPassIndex(graph, "PostProcessFinalize");
    EXPECT_LT(shadowIdx, shadowReadIdx);
    EXPECT_LT(skyboxIdx, postIdx);
    EXPECT_LT(postIdx, finalizeIdx);
}

TEST(FullFrameRenderGraphCompileTests, FullChain_PostProcessDeclaredBeforeScene_TopoSortRunsSceneFirst)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);
    const RenderGraphTextureHandle ping = graph.ImportTexture("Ping", nullptr, colorAndShader);
    const ShadowHandles shadow = ImportShadowAtlases(graph);

    AddPostProcessPass(graph, color, ping, 0, 1);
    graph.AddPass(std::make_unique<PostProcessFinalizeGraphPass>(ping));
    AddSkyboxPass(graph, color, depth);
    AddScenePass(graph, color, depth);
    graph.AddPass(std::make_unique<SceneShadowReadGraphPass>(shadow.directional, shadow.spot, shadow.point));
    graph.AddPass(std::make_unique<ShadowRenderGraphPass>(
        nullptr, nullptr, nullptr, shadow.directional, shadow.spot, shadow.point));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 6u);
    const size_t shadowIdx = FindCompiledPassIndex(graph, "Shadow");
    const size_t shadowReadIdx = FindCompiledPassIndex(graph, "SceneShadowRead");
    const size_t sceneIdx = FindCompiledPassIndex(graph, "Scene");
    const size_t skyboxIdx = FindCompiledPassIndex(graph, "Skybox");
    const size_t postIdx = FindCompiledPassIndex(graph, "PostProcess");
    const size_t finalizeIdx = FindCompiledPassIndex(graph, "PostProcessFinalize");
    EXPECT_LT(shadowIdx, shadowReadIdx);
    EXPECT_LT(sceneIdx, postIdx);
    EXPECT_LT(skyboxIdx, postIdx);
    EXPECT_LT(postIdx, finalizeIdx);
}

TEST(FullFrameRenderGraphCompileTests, FullChain_ShadowToSceneColor)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);
    const RenderGraphTextureHandle ping = graph.ImportTexture("Ping", nullptr, colorAndShader);
    const ShadowHandles shadow = ImportShadowAtlases(graph);

    graph.AddPass(std::make_unique<ShadowRenderGraphPass>(
        nullptr, nullptr, nullptr, shadow.directional, shadow.spot, shadow.point));
    graph.AddPass(std::make_unique<SceneShadowReadGraphPass>(shadow.directional, shadow.spot, shadow.point));
    AddScenePass(graph, color, depth);
    AddSkyboxPass(graph, color, depth);
    AddPostProcessPass(graph, color, ping, 0, 1);

    graph.Compile();

    const auto& skyboxCompiled = graph.GetCompiledPass(3);
    for (const auto& transition : skyboxCompiled.transitions)
    {
        if (transition.texture == color)
            EXPECT_EQ(transition.stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    const auto& postTransitions = graph.GetCompiledPass(4).transitions;
    bool foundSceneSrv = false;
    for (const auto& transition : postTransitions)
    {
        if (transition.texture == color && transition.stateAfter == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            foundSceneSrv = true;
    }
    EXPECT_TRUE(foundSceneSrv);
}

TEST(FullFrameRenderGraphCompileTests, FullChain_WithMsaaResolve)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);
    const RenderGraphTextureHandle resolved = graph.ImportTexture("ResolvedScene", nullptr,
        RenderGraphTextureUsage::ShaderResource);
    const RenderGraphTextureHandle ping = graph.ImportTexture("Ping", nullptr, colorAndShader);

    AddScenePass(graph, color, depth);
    AddSkyboxPass(graph, color, depth);
    graph.AddPass(std::make_unique<MsaaResolveGraphPass>(color, resolved, nullptr, nullptr));
    AddPostProcessPass(graph, resolved, ping, 0, 1);

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 4u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "Scene");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(1).passIndex).GetName(), "Skybox");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(2).passIndex).GetName(), "MsaaResolve");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(3).passIndex).GetName(), "PostProcess");

    const auto& resolveTransitions = graph.GetCompiledPass(2).transitions;
    ASSERT_EQ(resolveTransitions.size(), 2u);
    EXPECT_EQ(resolveTransitions[0].texture, color);
    EXPECT_EQ(resolveTransitions[0].stateAfter, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
    EXPECT_EQ(resolveTransitions[1].texture, resolved);
    EXPECT_EQ(resolveTransitions[1].stateAfter, D3D12_RESOURCE_STATE_RESOLVE_DEST);
}

TEST(FullFrameRenderGraphCompileTests, FullChain_NoPostProcess)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);
    const ShadowHandles shadow = ImportShadowAtlases(graph);

    graph.AddPass(std::make_unique<ShadowRenderGraphPass>(
        nullptr, nullptr, nullptr, shadow.directional, shadow.spot, shadow.point));
    graph.AddPass(std::make_unique<SceneShadowReadGraphPass>(shadow.directional, shadow.spot, shadow.point));
    AddScenePass(graph, color, depth);
    AddSkyboxPass(graph, color, depth);

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 4u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(3).passIndex).GetName(), "Skybox");
}

TEST(FullFrameRenderGraphCompileTests, SkyboxPass_NoClears)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);

    AddScenePass(graph, color, depth);
    AddSkyboxPass(graph, color, depth);

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 2u);
    EXPECT_EQ(graph.GetCompiledPass(1).clears.size(), 0u);
}
