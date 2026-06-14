#include "Runtime/Graphics/RenderGraph/DeferredLightingGraphPass.h"
#include "Runtime/Graphics/RenderGraph/GBufferRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/PostProcessFinalizeGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
#include "Runtime/Graphics/RenderGraph/SceneShadowReadGraphPass.h"
#include "Runtime/Graphics/RenderGraph/ShadowRenderGraphPass.h"

#include <cstring>

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

void AddGBufferPass(RenderGraph& graph,
    RenderGraphTextureHandle albedo,
    RenderGraphTextureHandle normal,
    RenderGraphTextureHandle material,
    RenderGraphTextureHandle emissive,
    RenderGraphTextureHandle depth)
{
    graph.AddPass(std::make_unique<GBufferRenderGraphPass>(
        albedo, normal, material, emissive, depth,
        MakeDummyHandle(0), MakeDummyHandle(1), MakeDummyHandle(2), MakeDummyHandle(3), MakeDummyHandle(4),
        nullptr,
        CD3DX12_VIEWPORT(0.0f, 0.0f, 1920.0f, 1080.0f),
        CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX),
        nullptr,
        nullptr));
}

void AddDeferredLightingPass(RenderGraph& graph,
    RenderGraphTextureHandle albedo,
    RenderGraphTextureHandle normal,
    RenderGraphTextureHandle material,
    RenderGraphTextureHandle emissive,
    RenderGraphTextureHandle depth,
    RenderGraphTextureHandle sceneColor)
{
    graph.AddPass(std::make_unique<DeferredLightingGraphPass>(
        albedo, normal, material, emissive, depth, sceneColor,
        MakeDummyHandle(0), MakeDummyHandle(1), MakeDummyHandle(2), MakeDummyHandle(3), MakeDummyHandle(4),
        MakeDummyHandle(5),
        CD3DX12_VIEWPORT(0.0f, 0.0f, 1920.0f, 1080.0f),
        CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX),
        nullptr,
        nullptr));
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

TEST(GBufferRenderGraphCompileTests, GBufferPass_EmitsFourColorDepthTransitionsAndClears)
{
    RenderGraph graph;
    const RenderGraphTextureUsage gbufferUsage =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle albedo = graph.ImportTexture("GBufferAlbedo", nullptr, gbufferUsage);
    const RenderGraphTextureHandle normal = graph.ImportTexture("GBufferNormal", nullptr, gbufferUsage);
    const RenderGraphTextureHandle material = graph.ImportTexture("GBufferMaterial", nullptr, gbufferUsage);
    const RenderGraphTextureHandle emissive = graph.ImportTexture("GBufferEmissive", nullptr, gbufferUsage);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);

    AddGBufferPass(graph, albedo, normal, material, emissive, depth);
    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 1u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "GBuffer");

    const auto& transitions = graph.GetCompiledPass(0).transitions;
    ASSERT_EQ(transitions.size(), 5u);
    EXPECT_EQ(transitions[0].texture, albedo);
    EXPECT_EQ(transitions[0].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);
    EXPECT_EQ(transitions[4].texture, depth);
    EXPECT_EQ(transitions[4].stateAfter, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    EXPECT_EQ(graph.GetCompiledPass(0).clears.size(), 5u);
}

TEST(GBufferRenderGraphCompileTests, DeferredLightingPass_ReadsGBufferAndDepthWritesSceneColor)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;
    const RenderGraphTextureUsage depthAndShader =
        RenderGraphTextureUsage::DepthAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle albedo = graph.ImportTexture("GBufferAlbedo", nullptr, colorAndShader);
    const RenderGraphTextureHandle normal = graph.ImportTexture("GBufferNormal", nullptr, colorAndShader);
    const RenderGraphTextureHandle material = graph.ImportTexture("GBufferMaterial", nullptr, colorAndShader);
    const RenderGraphTextureHandle emissive = graph.ImportTexture("GBufferEmissive", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr, depthAndShader);
    const RenderGraphTextureHandle sceneColor = graph.ImportTexture("SceneColor", nullptr, colorAndShader);

    graph.AddPass(std::make_unique<DeferredLightingGraphPass>(
        albedo, normal, material, emissive, depth, sceneColor,
        MakeDummyHandle(0), MakeDummyHandle(1), MakeDummyHandle(2), MakeDummyHandle(3), MakeDummyHandle(4),
        MakeDummyHandle(5),
        CD3DX12_VIEWPORT(0.0f, 0.0f, 1920.0f, 1080.0f),
        CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX),
        nullptr,
        nullptr));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 1u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "DeferredLighting");

    const auto& transitions = graph.GetCompiledPass(0).transitions;
    ASSERT_EQ(transitions.size(), 6u);
    EXPECT_EQ(transitions[0].texture, albedo);
    EXPECT_EQ(transitions[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(transitions[4].texture, depth);
    EXPECT_EQ(transitions[4].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(transitions[5].texture, sceneColor);
    EXPECT_EQ(transitions[5].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

TEST(GBufferRenderGraphCompileTests, FullDeferredChain_WithShadows_OrderAndBarriers)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;
    const RenderGraphTextureUsage depthAndShader =
        RenderGraphTextureUsage::DepthAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle sceneColor = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr, depthAndShader);
    const RenderGraphTextureHandle albedo = graph.ImportTexture("GBufferAlbedo", nullptr, colorAndShader);
    const RenderGraphTextureHandle normal = graph.ImportTexture("GBufferNormal", nullptr, colorAndShader);
    const RenderGraphTextureHandle material = graph.ImportTexture("GBufferMaterial", nullptr, colorAndShader);
    const RenderGraphTextureHandle emissive = graph.ImportTexture("GBufferEmissive", nullptr, colorAndShader);
    const ShadowHandles shadow = ImportShadowAtlases(graph);

    graph.AddPass(std::make_unique<ShadowRenderGraphPass>(
        nullptr, nullptr, nullptr, shadow.directional, shadow.spot, shadow.point));
    graph.AddPass(std::make_unique<SceneShadowReadGraphPass>(shadow.directional, shadow.spot, shadow.point));
    AddGBufferPass(graph, albedo, normal, material, emissive, depth);
    AddDeferredLightingPass(graph, albedo, normal, material, emissive, depth, sceneColor);
    graph.AddPass(std::make_unique<PostProcessFinalizeGraphPass>(sceneColor));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 5u);

    const size_t shadowIdx = FindCompiledPassIndex(graph, "Shadow");
    const size_t shadowReadIdx = FindCompiledPassIndex(graph, "SceneShadowRead");
    const size_t gbufferIdx = FindCompiledPassIndex(graph, "GBuffer");
    const size_t lightingIdx = FindCompiledPassIndex(graph, "DeferredLighting");
    const size_t finalizeIdx = FindCompiledPassIndex(graph, "PostProcessFinalize");
    EXPECT_LT(shadowIdx, shadowReadIdx);
    EXPECT_LT(shadowReadIdx, gbufferIdx);
    EXPECT_LT(gbufferIdx, lightingIdx);
    EXPECT_LT(lightingIdx, finalizeIdx);
    EXPECT_EQ(FindCompiledPassIndex(graph, "MsaaResolve"), graph.GetCompiledPassCount());

    const auto& shadowReadTransitions = graph.GetCompiledPass(shadowReadIdx).transitions;
    ASSERT_EQ(shadowReadTransitions.size(), 3u);
    EXPECT_EQ(shadowReadTransitions[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(shadowReadTransitions[1].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(shadowReadTransitions[2].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    const auto& gbufferTransitions = graph.GetCompiledPass(gbufferIdx).transitions;
    ASSERT_EQ(gbufferTransitions.size(), 5u);
    EXPECT_EQ(gbufferTransitions[0].texture, albedo);
    EXPECT_EQ(gbufferTransitions[0].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);
    EXPECT_EQ(gbufferTransitions[1].texture, normal);
    EXPECT_EQ(gbufferTransitions[1].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);
    EXPECT_EQ(gbufferTransitions[2].texture, material);
    EXPECT_EQ(gbufferTransitions[2].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);
    EXPECT_EQ(gbufferTransitions[3].texture, emissive);
    EXPECT_EQ(gbufferTransitions[3].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);
    EXPECT_EQ(gbufferTransitions[4].texture, depth);
    EXPECT_EQ(gbufferTransitions[4].stateAfter, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    EXPECT_EQ(graph.GetCompiledPass(gbufferIdx).clears.size(), 5u);

    const auto& lightingTransitions = graph.GetCompiledPass(lightingIdx).transitions;
    ASSERT_EQ(lightingTransitions.size(), 6u);
    EXPECT_EQ(lightingTransitions[0].texture, albedo);
    EXPECT_EQ(lightingTransitions[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(lightingTransitions[1].texture, normal);
    EXPECT_EQ(lightingTransitions[1].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(lightingTransitions[2].texture, material);
    EXPECT_EQ(lightingTransitions[2].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(lightingTransitions[3].texture, emissive);
    EXPECT_EQ(lightingTransitions[3].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(lightingTransitions[4].texture, depth);
    EXPECT_EQ(lightingTransitions[4].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    EXPECT_EQ(lightingTransitions[5].texture, sceneColor);
    EXPECT_EQ(lightingTransitions[5].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

TEST(GBufferRenderGraphCompileTests, FullDeferredChain_ShadowReadDeclaredFirst_TopoSortsShadowFirst)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;
    const RenderGraphTextureUsage depthAndShader =
        RenderGraphTextureUsage::DepthAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle sceneColor = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr, depthAndShader);
    const RenderGraphTextureHandle albedo = graph.ImportTexture("GBufferAlbedo", nullptr, colorAndShader);
    const RenderGraphTextureHandle normal = graph.ImportTexture("GBufferNormal", nullptr, colorAndShader);
    const RenderGraphTextureHandle material = graph.ImportTexture("GBufferMaterial", nullptr, colorAndShader);
    const RenderGraphTextureHandle emissive = graph.ImportTexture("GBufferEmissive", nullptr, colorAndShader);
    const ShadowHandles shadow = ImportShadowAtlases(graph);

    graph.AddPass(std::make_unique<SceneShadowReadGraphPass>(shadow.directional, shadow.spot, shadow.point));
    AddDeferredLightingPass(graph, albedo, normal, material, emissive, depth, sceneColor);
    graph.AddPass(std::make_unique<PostProcessFinalizeGraphPass>(sceneColor));
    AddGBufferPass(graph, albedo, normal, material, emissive, depth);
    graph.AddPass(std::make_unique<ShadowRenderGraphPass>(
        nullptr, nullptr, nullptr, shadow.directional, shadow.spot, shadow.point));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 5u);
    const size_t shadowIdx = FindCompiledPassIndex(graph, "Shadow");
    const size_t shadowReadIdx = FindCompiledPassIndex(graph, "SceneShadowRead");
    const size_t gbufferIdx = FindCompiledPassIndex(graph, "GBuffer");
    const size_t lightingIdx = FindCompiledPassIndex(graph, "DeferredLighting");
    const size_t finalizeIdx = FindCompiledPassIndex(graph, "PostProcessFinalize");
    EXPECT_LT(shadowIdx, shadowReadIdx);
    EXPECT_LT(gbufferIdx, lightingIdx);
    EXPECT_LT(lightingIdx, finalizeIdx);
}
