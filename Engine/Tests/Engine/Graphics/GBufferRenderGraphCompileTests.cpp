#include "Runtime/Graphics/RenderGraph/DeferredLightingGraphPass.h"
#include "Runtime/Graphics/RenderGraph/GBufferRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraph.h"

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
