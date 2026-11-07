#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/TemporalHistory.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

using namespace DeltaEngine;

namespace
{
std::shared_ptr<DirectX12Texture> MakeFakeTexture()
{
    auto owner = std::make_shared<int>(0);
    return std::shared_ptr<DirectX12Texture>(owner, reinterpret_cast<DirectX12Texture*>(owner.get()));
}

class FakeSceneWritePass final : public RenderGraphPass
{
public:
    explicit FakeSceneWritePass(RenderGraphTextureHandle color) : m_color(color) {}

    const char* GetName() const override { return "FakeScene"; }
    void Setup(RenderGraphBuilder& builder) override { builder.Write(m_color, D3D12_RESOURCE_STATE_RENDER_TARGET); }
    void Execute(const RenderGraphContext&) const override {}

private:
    RenderGraphTextureHandle m_color;
};

class FakeTemporalResolvePass final : public RenderGraphPass
{
public:
    FakeTemporalResolvePass(RenderGraphTextureHandle color, TemporalHistoryHandles history)
        : m_color(color), m_history(history)
    {
    }

    const char* GetName() const override { return "FakeTemporalResolve"; }
    void Setup(RenderGraphBuilder& builder) override
    {
        builder.Read(m_color, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        builder.Read(m_history.read, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        builder.Write(m_history.write, D3D12_RESOURCE_STATE_RENDER_TARGET);
    }
    void Execute(const RenderGraphContext&) const override {}

private:
    RenderGraphTextureHandle m_color;
    TemporalHistoryHandles m_history;
};

TemporalHistory MakeHistory()
{
    TemporalHistory history;
    history.SetTextureFactory([](const D3D12_RESOURCE_DESC&, const std::string&) { return MakeFakeTexture(); });
    history.Resize(64, 64);
    return history;
}

TemporalHistoryHandles BuildFrame(RenderGraph& graph, const TemporalHistory& history)
{
    const RenderGraphTextureUsage colorAndShader =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;
    const RenderGraphTextureHandle color = graph.ImportTexture("SceneColor", nullptr, colorAndShader);
    const TemporalHistoryHandles handles = history.Import(graph);
    graph.AddPass(std::make_unique<FakeSceneWritePass>(color));
    graph.AddPass(std::make_unique<FakeTemporalResolvePass>(color, handles));
    graph.Compile();
    return handles;
}

bool HasTransition(const RenderGraphCompiledPass& pass, RenderGraphTextureHandle texture, D3D12_RESOURCE_STATES state)
{
    for (const RenderGraphResourceTransition& transition : pass.transitions)
    {
        if (transition.texture == texture && transition.stateAfter == state)
            return true;
    }
    return false;
}

void ExpectResolveBarriers(const RenderGraph& graph, const TemporalHistoryHandles& handles)
{
    ASSERT_EQ(graph.GetCompiledPassCount(), 2u);
    const RenderGraphCompiledPass& resolve = graph.GetCompiledPass(1);
    EXPECT_STREQ(graph.GetPass(resolve.passIndex).GetName(), "FakeTemporalResolve");
    EXPECT_TRUE(HasTransition(resolve, handles.read, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    EXPECT_TRUE(HasTransition(resolve, handles.write, D3D12_RESOURCE_STATE_RENDER_TARGET));
    EXPECT_TRUE(resolve.clears.empty());

    const RenderGraphCompiledPass& scene = graph.GetCompiledPass(0);
    EXPECT_FALSE(HasTransition(scene, handles.read, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    EXPECT_FALSE(HasTransition(scene, handles.write, D3D12_RESOURCE_STATE_RENDER_TARGET));
}
} // namespace

TEST(TemporalHistoryRenderGraphCompileTests, Compile_EmitsHistoryReadAndWriteBarriers)
{
    const TemporalHistory history = MakeHistory();
    RenderGraph graph;

    const TemporalHistoryHandles handles = BuildFrame(graph, history);

    ASSERT_TRUE(handles.read.IsValid());
    ASSERT_TRUE(handles.write.IsValid());
    ExpectResolveBarriers(graph, handles);
}

TEST(TemporalHistoryRenderGraphCompileTests, Import_RolesSwapAcrossFrames)
{
    TemporalHistory history = MakeHistory();
    RenderGraph graph;

    const TemporalHistoryHandles first = BuildFrame(graph, history);
    const auto firstRead = graph.GetImportedTexture(first.read).texture;
    const auto firstWrite = graph.GetImportedTexture(first.write).texture;
    ExpectResolveBarriers(graph, first);

    history.EndFrame(true);
    graph.Reset();

    const TemporalHistoryHandles second = BuildFrame(graph, history);
    EXPECT_EQ(graph.GetImportedTexture(second.read).texture, firstWrite);
    EXPECT_EQ(graph.GetImportedTexture(second.write).texture, firstRead);
    ExpectResolveBarriers(graph, second);
}

TEST(TemporalHistoryRenderGraphCompileTests, Import_IsNotTransient)
{
    const TemporalHistory history = MakeHistory();
    RenderGraph graph;
    ASSERT_EQ(graph.GetTransientPool(), nullptr);

    const TemporalHistoryHandles handles = history.Import(graph);

    ASSERT_TRUE(handles.read.IsValid());
    ASSERT_TRUE(handles.write.IsValid());
    EXPECT_FALSE(graph.GetImportedTexture(handles.read).transient);
    EXPECT_FALSE(graph.GetImportedTexture(handles.write).transient);
    EXPECT_EQ(graph.GetImportedTexture(handles.read).texture, history.GetReadTexture());
    EXPECT_EQ(graph.GetImportedTexture(handles.write).texture, history.GetWriteTexture());
}

TEST(TemporalHistoryRenderGraphCompileTests, Import_WithoutTextures_ReturnsInvalidHandles)
{
    const TemporalHistory history;
    RenderGraph graph;

    const TemporalHistoryHandles handles = history.Import(graph);

    EXPECT_FALSE(handles.read.IsValid());
    EXPECT_FALSE(handles.write.IsValid());
    EXPECT_EQ(graph.GetImportedTextureCount(), 0u);
}
