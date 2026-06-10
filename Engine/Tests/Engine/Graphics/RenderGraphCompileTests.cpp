#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphPass.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace DeltaEngine;

namespace
{
struct AccessDecl
{
    RenderGraphTextureHandle texture;
    D3D12_RESOURCE_STATES state;
    RenderGraphAccessType type;
};

class RecordingPass final : public RenderGraphPass
{
public:
    RecordingPass(std::string name, std::vector<AccessDecl> accesses)
    : m_name(std::move(name)), m_accesses(std::move(accesses))
    {
    }

    const char* GetName() const override { return m_name.c_str(); }

    void Setup(RenderGraphBuilder& builder) override
    {
        for (const AccessDecl& access : m_accesses)
        {
            if (access.type == RenderGraphAccessType::Write)
            {
                builder.Write(access.texture, access.state);
            }
            else
            {
                builder.Read(access.texture, access.state);
            }
        }
    }

    void Execute(const RenderGraphContext&) const override {}

private:
    std::string m_name;
    std::vector<AccessDecl> m_accesses;
};
} // namespace

TEST(RenderGraphCompileTests, EmptyGraph_ProducesNoCompiledPasses)
{
    RenderGraph graph;
    graph.Compile();
    EXPECT_EQ(graph.GetCompiledPassCount(), 0u);
}

TEST(RenderGraphCompileTests, IndependentPasses_PreserveDeclarationOrder)
{
    RenderGraph graph;
    const RenderGraphTextureHandle a =
        graph.ImportTexture("A", nullptr, RenderGraphTextureUsage::ColorAttachment);
    const RenderGraphTextureHandle b =
        graph.ImportTexture("B", nullptr, RenderGraphTextureUsage::ColorAttachment);

    graph.AddPass(std::make_unique<RecordingPass>(
        "First", std::vector<AccessDecl>{ { a, D3D12_RESOURCE_STATE_RENDER_TARGET, RenderGraphAccessType::Write } }));
    graph.AddPass(std::make_unique<RecordingPass>(
        "Second", std::vector<AccessDecl>{ { b, D3D12_RESOURCE_STATE_RENDER_TARGET, RenderGraphAccessType::Write } }));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 2u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "First");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(1).passIndex).GetName(), "Second");

    ASSERT_EQ(graph.GetCompiledPass(0).transitions.size(), 1u);
    EXPECT_EQ(graph.GetCompiledPass(0).transitions[0].texture, a);
    EXPECT_EQ(graph.GetCompiledPass(0).transitions[0].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

TEST(RenderGraphCompileTests, ProducerConsumer_OrdersAndEmitsTransition)
{
    RenderGraph graph;
    const RenderGraphTextureHandle tex =
        graph.ImportTexture("SceneColor", nullptr, RenderGraphTextureUsage::ColorAttachment);

    graph.AddPass(std::make_unique<RecordingPass>(
        "Producer",
        std::vector<AccessDecl>{ { tex, D3D12_RESOURCE_STATE_RENDER_TARGET, RenderGraphAccessType::Write } }));
    graph.AddPass(std::make_unique<RecordingPass>(
        "Consumer",
        std::vector<AccessDecl>{ { tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RenderGraphAccessType::Read } }));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 2u);
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "Producer");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(1).passIndex).GetName(), "Consumer");

    ASSERT_EQ(graph.GetCompiledPass(0).transitions.size(), 1u);
    EXPECT_EQ(graph.GetCompiledPass(0).transitions[0].stateAfter, D3D12_RESOURCE_STATE_RENDER_TARGET);

    ASSERT_EQ(graph.GetCompiledPass(1).transitions.size(), 1u);
    EXPECT_EQ(graph.GetCompiledPass(1).transitions[0].texture, tex);
    EXPECT_EQ(graph.GetCompiledPass(1).transitions[0].stateAfter, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

TEST(RenderGraphCompileTests, UnchangedState_EmitsNoDuplicateTransition)
{
    RenderGraph graph;
    const RenderGraphTextureHandle tex =
        graph.ImportTexture("Shared", nullptr, RenderGraphTextureUsage::ShaderResource);

    graph.AddPass(std::make_unique<RecordingPass>(
        "Writer",
        std::vector<AccessDecl>{ { tex, D3D12_RESOURCE_STATE_RENDER_TARGET, RenderGraphAccessType::Write } }));
    graph.AddPass(std::make_unique<RecordingPass>(
        "ReaderA",
        std::vector<AccessDecl>{ { tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RenderGraphAccessType::Read } }));
    graph.AddPass(std::make_unique<RecordingPass>(
        "ReaderB",
        std::vector<AccessDecl>{ { tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RenderGraphAccessType::Read } }));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 3u);
    // Second reader already sees PIXEL_SHADER_RESOURCE => no transition emitted.
    EXPECT_TRUE(graph.GetCompiledPass(2).transitions.empty());
}

TEST(RenderGraphCompileTests, DependencyAcrossDeclarationOrder_TopoSortsProducerFirst)
{
    RenderGraph graph;
    const RenderGraphTextureHandle tex =
        graph.ImportTexture("Buffer", nullptr, RenderGraphTextureUsage::ColorAttachment);

    // Consumer declared before producer; topo sort must still run producer first.
    graph.AddPass(std::make_unique<RecordingPass>(
        "Consumer",
        std::vector<AccessDecl>{ { tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RenderGraphAccessType::Read } }));
    graph.AddPass(std::make_unique<RecordingPass>(
        "Producer",
        std::vector<AccessDecl>{ { tex, D3D12_RESOURCE_STATE_RENDER_TARGET, RenderGraphAccessType::Write } }));

    graph.Compile();

    ASSERT_EQ(graph.GetCompiledPassCount(), 2u);
    // The reader depends on the writer regardless of declaration order, so the
    // topological sort schedules the producer first.
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(0).passIndex).GetName(), "Producer");
    EXPECT_STREQ(graph.GetPass(graph.GetCompiledPass(1).passIndex).GetName(), "Consumer");
}

TEST(RenderGraphCompileTests, Cycle_TriggersAssert)
{
    RenderGraph graph;
    const RenderGraphTextureHandle x =
        graph.ImportTexture("X", nullptr, RenderGraphTextureUsage::ColorAttachment);
    const RenderGraphTextureHandle y =
        graph.ImportTexture("Y", nullptr, RenderGraphTextureUsage::ColorAttachment);

    // PassA writes X then reads Y; PassB writes Y then reads X => cycle.
    graph.AddPass(std::make_unique<RecordingPass>(
        "PassA",
        std::vector<AccessDecl>{ { x, D3D12_RESOURCE_STATE_RENDER_TARGET, RenderGraphAccessType::Write },
            { y, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RenderGraphAccessType::Read } }));
    graph.AddPass(std::make_unique<RecordingPass>(
        "PassB",
        std::vector<AccessDecl>{ { y, D3D12_RESOURCE_STATE_RENDER_TARGET, RenderGraphAccessType::Write },
            { x, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, RenderGraphAccessType::Read } }));

    EXPECT_DEATH(graph.Compile(), "");
}
