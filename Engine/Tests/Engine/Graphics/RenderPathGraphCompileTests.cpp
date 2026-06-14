#include "Runtime/Graphics/RenderGraph/DeferredLightingGraphPass.h"
#include "Runtime/Graphics/RenderGraph/GBufferRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/MsaaResolveGraphPass.h"
#include "Runtime/Graphics/RenderGraph/PostProcessFinalizeGraphPass.h"
#include "Runtime/Graphics/RenderGraph/PostProcessRenderGraphPass.h"
#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
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

D3D12_CPU_DESCRIPTOR_HANDLE MakeDummyHandle(UINT index)
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle{};
    handle.ptr = static_cast<SIZE_T>(index);
    return handle;
}

void AddShadowPasses(RenderGraph& graph, const ShadowHandles& shadow)
{
    graph.AddPass(std::make_unique<ShadowRenderGraphPass>(
        nullptr, nullptr, nullptr, shadow.directional, shadow.spot, shadow.point));
    graph.AddPass(std::make_unique<SceneShadowReadGraphPass>(shadow.directional, shadow.spot, shadow.point));
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

void AddGBufferPass(RenderGraph& graph,
    RenderGraphTextureHandle albedo,
    RenderGraphTextureHandle normal,
    RenderGraphTextureHandle material,
    RenderGraphTextureHandle emissive,
    RenderGraphTextureHandle depth)
{
    graph.AddPass(std::make_unique<GBufferRenderGraphPass>(
        albedo, normal, material, emissive, depth,
        MakeDummyHandle(10), MakeDummyHandle(11), MakeDummyHandle(12), MakeDummyHandle(13), MakeDummyHandle(14),
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

void AddPostProcessPass(RenderGraph& graph, RenderGraphTextureHandle input, RenderGraphTextureHandle output)
{
    graph.AddPass(std::make_unique<PostProcessRenderGraphPass>(
        nullptr, input, output, MakeDummyHandle(0), MakeDummyHandle(1), 1920, 1080));
}

void BuildSharedTail(RenderGraph& graph, const ShadowHandles& shadow,
    RenderGraphTextureHandle postInput, RenderGraphTextureHandle ping)
{
    AddShadowPasses(graph, shadow);
    AddPostProcessPass(graph, postInput, ping);
    graph.AddPass(std::make_unique<PostProcessFinalizeGraphPass>(ping));
}

void BuildForwardMiddle(RenderGraph& graph, RenderGraphTextureHandle color, RenderGraphTextureHandle depth,
    RenderGraphTextureHandle resolved, bool includeMsaaResolve)
{
    AddScenePass(graph, color, depth);
    AddSkyboxPass(graph, color, depth);
    if (includeMsaaResolve)
        graph.AddPass(std::make_unique<MsaaResolveGraphPass>(color, resolved, nullptr, nullptr));
}

void BuildDeferredMiddle(RenderGraph& graph,
    RenderGraphTextureHandle albedo,
    RenderGraphTextureHandle normal,
    RenderGraphTextureHandle material,
    RenderGraphTextureHandle emissive,
    RenderGraphTextureHandle depth,
    RenderGraphTextureHandle sceneColor,
    bool includeSkybox)
{
    AddGBufferPass(graph, albedo, normal, material, emissive, depth);
    AddDeferredLightingPass(graph, albedo, normal, material, emissive, depth, sceneColor);
    if (includeSkybox)
        AddSkyboxPass(graph, sceneColor, depth);
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

bool HasPass(const RenderGraph& graph, const char* passName)
{
    return FindCompiledPassIndex(graph, passName) < graph.GetCompiledPassCount();
}
} // namespace

TEST(RenderPathGraphCompileTests, RenderPathGraphStructure_Forward_HasSceneAndMsaaResolve)
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
    const ShadowHandles shadow = ImportShadowAtlases(graph);

    BuildSharedTail(graph, shadow, resolved, ping);
    BuildForwardMiddle(graph, color, depth, resolved, true);

    graph.Compile();

    EXPECT_TRUE(HasPass(graph, "Scene"));
    EXPECT_TRUE(HasPass(graph, "Skybox"));
    EXPECT_TRUE(HasPass(graph, "MsaaResolve"));
    EXPECT_FALSE(HasPass(graph, "GBuffer"));
    EXPECT_FALSE(HasPass(graph, "DeferredLighting"));
}

TEST(RenderPathGraphCompileTests, RenderPathGraphStructure_Deferred_HasGBufferAndLighting)
{
    RenderGraph graph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;
    const RenderGraphTextureUsage depthAndShader =
        RenderGraphTextureUsage::DepthAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle depth = graph.ImportTexture("SceneDepth", nullptr, depthAndShader);
    const RenderGraphTextureHandle albedo = graph.ImportTexture("GBufferAlbedo", nullptr, colorAndShader);
    const RenderGraphTextureHandle normal = graph.ImportTexture("GBufferNormal", nullptr, colorAndShader);
    const RenderGraphTextureHandle material = graph.ImportTexture("GBufferMaterial", nullptr, colorAndShader);
    const RenderGraphTextureHandle emissive = graph.ImportTexture("GBufferEmissive", nullptr, colorAndShader);
    const RenderGraphTextureHandle ping = graph.ImportTexture("Ping", nullptr, colorAndShader);
    const ShadowHandles shadow = ImportShadowAtlases(graph);

    BuildSharedTail(graph, shadow, color, ping);
    BuildDeferredMiddle(graph, albedo, normal, material, emissive, depth, color, true);

    graph.Compile();

    EXPECT_TRUE(HasPass(graph, "GBuffer"));
    EXPECT_TRUE(HasPass(graph, "DeferredLighting"));
    EXPECT_TRUE(HasPass(graph, "Skybox"));
    EXPECT_FALSE(HasPass(graph, "Scene"));
    EXPECT_FALSE(HasPass(graph, "MsaaResolve"));
}

TEST(RenderPathGraphCompileTests, RenderPathGraphStructure_SharedTail_Matches)
{
    RenderGraph forwardGraph;
    RenderGraph deferredGraph;
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;
    const RenderGraphTextureUsage depthAndShader =
        RenderGraphTextureUsage::DepthAttachment | RenderGraphTextureUsage::ShaderResource;

    const RenderGraphTextureHandle fwdColor = forwardGraph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle fwdDepth = forwardGraph.ImportTexture("SceneDepth", nullptr,
        RenderGraphTextureUsage::DepthAttachment);
    const RenderGraphTextureHandle fwdResolved = forwardGraph.ImportTexture("ResolvedScene", nullptr,
        RenderGraphTextureUsage::ShaderResource);
    const RenderGraphTextureHandle fwdPing = forwardGraph.ImportTexture("Ping", nullptr, colorAndShader);
    const ShadowHandles fwdShadow = ImportShadowAtlases(forwardGraph);

    BuildSharedTail(forwardGraph, fwdShadow, fwdResolved, fwdPing);
    BuildForwardMiddle(forwardGraph, fwdColor, fwdDepth, fwdResolved, true);

    const RenderGraphTextureHandle defColor = deferredGraph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const RenderGraphTextureHandle defDepth = deferredGraph.ImportTexture("SceneDepth", nullptr, depthAndShader);
    const RenderGraphTextureHandle defAlbedo = deferredGraph.ImportTexture("GBufferAlbedo", nullptr, colorAndShader);
    const RenderGraphTextureHandle defNormal = deferredGraph.ImportTexture("GBufferNormal", nullptr, colorAndShader);
    const RenderGraphTextureHandle defMaterial = deferredGraph.ImportTexture("GBufferMaterial", nullptr, colorAndShader);
    const RenderGraphTextureHandle defEmissive = deferredGraph.ImportTexture("GBufferEmissive", nullptr, colorAndShader);
    const RenderGraphTextureHandle defPing = deferredGraph.ImportTexture("Ping", nullptr, colorAndShader);
    const ShadowHandles defShadow = ImportShadowAtlases(deferredGraph);

    BuildSharedTail(deferredGraph, defShadow, defColor, defPing);
    BuildDeferredMiddle(deferredGraph, defAlbedo, defNormal, defMaterial, defEmissive, defDepth, defColor, false);

    forwardGraph.Compile();
    deferredGraph.Compile();

    const size_t fwdShadowIdx = FindCompiledPassIndex(forwardGraph, "Shadow");
    const size_t fwdShadowReadIdx = FindCompiledPassIndex(forwardGraph, "SceneShadowRead");
    const size_t fwdPostIdx = FindCompiledPassIndex(forwardGraph, "PostProcess");
    const size_t fwdFinalizeIdx = FindCompiledPassIndex(forwardGraph, "PostProcessFinalize");

    const size_t defShadowIdx = FindCompiledPassIndex(deferredGraph, "Shadow");
    const size_t defShadowReadIdx = FindCompiledPassIndex(deferredGraph, "SceneShadowRead");
    const size_t defPostIdx = FindCompiledPassIndex(deferredGraph, "PostProcess");
    const size_t defFinalizeIdx = FindCompiledPassIndex(deferredGraph, "PostProcessFinalize");

    EXPECT_LT(fwdShadowIdx, fwdShadowReadIdx);
    EXPECT_LT(fwdPostIdx, fwdFinalizeIdx);
    EXPECT_LT(defShadowIdx, defShadowReadIdx);
    EXPECT_LT(defPostIdx, defFinalizeIdx);

    const auto& fwdPostTransitions = forwardGraph.GetCompiledPass(fwdPostIdx).transitions;
    bool fwdFoundSrv = false;
    for (const auto& transition : fwdPostTransitions)
    {
        if (transition.texture == fwdResolved
            && transition.stateAfter == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            fwdFoundSrv = true;
    }
    EXPECT_TRUE(fwdFoundSrv);

    const auto& defPostTransitions = deferredGraph.GetCompiledPass(defPostIdx).transitions;
    bool defFoundSrv = false;
    for (const auto& transition : defPostTransitions)
    {
        if (transition.texture == defColor
            && transition.stateAfter == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            defFoundSrv = true;
    }
    EXPECT_TRUE(defFoundSrv);
}
