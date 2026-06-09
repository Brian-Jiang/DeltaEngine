#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
class DummyRenderGraphPass final : public RenderGraphPass
{
public:
    const char* GetName() const override { return "DummyPass"; }
    void Setup(RenderGraphBuilder&) override {}
    void Execute(const RenderGraphContext&) const override {}
};
}

TEST(RenderGraphTests, EmptyGraph_HasZeroPassesAndTextures)
{
    RenderGraph graph;
    EXPECT_EQ(graph.GetPassCount(), 0u);
    EXPECT_EQ(graph.GetImportedTextureCount(), 0u);
}

TEST(RenderGraphTests, AddPass_RegistersPassByName)
{
    RenderGraph graph;
    graph.AddPass(std::make_unique<DummyRenderGraphPass>());

    ASSERT_EQ(graph.GetPassCount(), 1u);
    EXPECT_STREQ(graph.GetPass(0).GetName(), "DummyPass");
}

TEST(RenderGraphTests, ImportTexture_RegistersExternalResource)
{
    RenderGraph graph;
    const auto usage = RenderGraphTextureUsage::ShaderResource | RenderGraphTextureUsage::CopyDst;
    const RenderGraphTextureHandle handle =
        graph.ImportTexture("SceneColor", nullptr, usage);

    ASSERT_TRUE(handle.IsValid());
    ASSERT_EQ(graph.GetImportedTextureCount(), 1u);

    const RenderGraphTexture& imported = graph.GetImportedTexture(handle);
    EXPECT_EQ(imported.name, "SceneColor");
    EXPECT_EQ(imported.texture, nullptr);
    EXPECT_EQ(imported.usage, usage);
}

TEST(RenderGraphTests, ImportTexture_RejectsDuplicateName)
{
    RenderGraph graph;
    graph.ImportTexture("SceneColor", nullptr, RenderGraphTextureUsage::ShaderResource);
    EXPECT_DEATH(graph.ImportTexture("SceneColor", nullptr, RenderGraphTextureUsage::ShaderResource), "");
}

TEST(RenderGraphTests, FindImportedTexture_ReturnsHandleByName)
{
    RenderGraph graph;
    const RenderGraphTextureHandle imported =
        graph.ImportTexture("DepthBuffer", nullptr, RenderGraphTextureUsage::DepthAttachment);

    const RenderGraphTextureHandle found = graph.FindImportedTexture("DepthBuffer");
    ASSERT_TRUE(found.IsValid());
    EXPECT_EQ(found, imported);
    EXPECT_EQ(graph.GetImportedTexture(found).usage, RenderGraphTextureUsage::DepthAttachment);
}

TEST(RenderGraphTests, Reset_ClearsPassesTexturesAndCompiledOutput)
{
    RenderGraph graph;
    graph.AddPass(std::make_unique<DummyRenderGraphPass>());
    graph.ImportTexture("SceneColor", nullptr, RenderGraphTextureUsage::ShaderResource);
    graph.Compile();

    ASSERT_EQ(graph.GetPassCount(), 1u);
    ASSERT_EQ(graph.GetCompiledPassCount(), 1u);

    graph.Reset();

    EXPECT_EQ(graph.GetPassCount(), 0u);
    EXPECT_EQ(graph.GetImportedTextureCount(), 0u);
    EXPECT_EQ(graph.GetCompiledPassCount(), 0u);
}
