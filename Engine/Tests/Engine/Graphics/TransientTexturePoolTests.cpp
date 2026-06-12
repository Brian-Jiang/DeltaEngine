#include "Runtime/Graphics/RenderGraph/RenderGraph.h"
#include "Runtime/Graphics/RenderGraph/TransientTexturePool.h"

#include <d3dx12.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

using namespace DeltaEngine;

namespace
{
// The pool never dereferences pooled textures, so tests can hand out opaque
// non-null pointers without constructing a real GPU resource.
std::shared_ptr<DirectX12Texture> MakeFakeTexture()
{
    auto owner = std::make_shared<int>(0);
    return std::shared_ptr<DirectX12Texture>(owner, reinterpret_cast<DirectX12Texture*>(owner.get()));
}

struct CountingFactory
{
    int callCount = 0;
    std::vector<std::string> names;

    TransientTexturePool::TextureFactory Bind()
    {
        return [this](const D3D12_RESOURCE_DESC&, const std::string& name)
        {
            ++callCount;
            names.push_back(name);
            return MakeFakeTexture();
        };
    }
};

D3D12_RESOURCE_DESC MakeDesc(UINT width, UINT height, DXGI_FORMAT format = DXGI_FORMAT_R16G16B16A16_FLOAT)
{
    return CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_NONE);
}

D3D12_RESOURCE_DESC MakeGBufferDesc(UINT width, UINT height, DXGI_FORMAT format)
{
    return CD3DX12_RESOURCE_DESC::Tex2D(
        format, width, height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
}
} // namespace

TEST(TransientTexturePoolTests, Acquire_CreatesViaFactoryAndTracksActive)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    auto texture = pool.Acquire(MakeDesc(64, 64), "TestTexture");

    ASSERT_NE(texture, nullptr);
    EXPECT_EQ(factory.callCount, 1);
    ASSERT_EQ(factory.names.size(), 1u);
    EXPECT_EQ(factory.names[0], "TestTexture");
    EXPECT_EQ(pool.GetActiveCount(), 1u);
    EXPECT_EQ(pool.GetFreeCount(), 0u);
    EXPECT_EQ(pool.GetInFlightCount(), 0u);
}

TEST(TransientTexturePoolTests, Acquire_DistinctDescriptions_CreateDistinctTextures)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    auto a = pool.Acquire(MakeDesc(64, 64), "A");
    auto b = pool.Acquire(MakeDesc(128, 64), "B");

    EXPECT_NE(a, b);
    EXPECT_EQ(factory.callCount, 2);
    EXPECT_EQ(pool.GetActiveCount(), 2u);
}

TEST(TransientTexturePoolTests, RetireFrame_MovesActiveToInFlight)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    pool.Acquire(MakeDesc(64, 64), "A");
    pool.RetireFrame(5);

    EXPECT_EQ(pool.GetActiveCount(), 0u);
    EXPECT_EQ(pool.GetInFlightCount(), 1u);
    EXPECT_EQ(pool.GetFreeCount(), 0u);
}

TEST(TransientTexturePoolTests, BeginFrame_RecyclesOnlyCompletedFences)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    pool.Acquire(MakeDesc(64, 64), "A");
    pool.RetireFrame(5);

    pool.BeginFrame(4);
    EXPECT_EQ(pool.GetInFlightCount(), 1u);
    EXPECT_EQ(pool.GetFreeCount(), 0u);

    pool.BeginFrame(5);
    EXPECT_EQ(pool.GetInFlightCount(), 0u);
    EXPECT_EQ(pool.GetFreeCount(), 1u);
}

TEST(TransientTexturePoolTests, Acquire_ReusesRecycledTextureForMatchingDesc)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    auto first = pool.Acquire(MakeDesc(64, 64), "A");
    pool.RetireFrame(1);
    pool.BeginFrame(1);

    auto second = pool.Acquire(MakeDesc(64, 64), "A");

    EXPECT_EQ(first, second);
    EXPECT_EQ(factory.callCount, 1);
    EXPECT_EQ(pool.GetActiveCount(), 1u);
    EXPECT_EQ(pool.GetFreeCount(), 0u);
}

TEST(TransientTexturePoolTests, Acquire_MismatchedDesc_DoesNotReuse)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    auto first = pool.Acquire(MakeDesc(64, 64), "A");
    pool.RetireFrame(1);
    pool.BeginFrame(1);

    auto second = pool.Acquire(MakeDesc(64, 64, DXGI_FORMAT_R8G8B8A8_UNORM), "B");

    EXPECT_NE(first, second);
    EXPECT_EQ(factory.callCount, 2);
    EXPECT_EQ(pool.GetFreeCount(), 1u);
    EXPECT_EQ(pool.GetActiveCount(), 1u);
}

TEST(TransientTexturePoolTests, BeginFrame_TrimsEntriesIdleForMaxIdleFrames)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    pool.Acquire(MakeDesc(64, 64), "A");
    pool.RetireFrame(1);
    pool.BeginFrame(1);
    ASSERT_EQ(pool.GetFreeCount(), 1u);

    for (uint32_t i = 0; i < TransientTexturePool::kMaxIdleFrames; ++i)
    {
        pool.BeginFrame(1);
        EXPECT_EQ(pool.GetFreeCount(), 1u);
    }

    pool.BeginFrame(1);
    EXPECT_EQ(pool.GetFreeCount(), 0u);
    EXPECT_EQ(pool.GetTotalCount(), 0u);
}

TEST(TransientTexturePoolTests, Reacquire_ResetsIdleAging)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    auto first = pool.Acquire(MakeDesc(64, 64), "A");
    pool.RetireFrame(1);
    pool.BeginFrame(1);

    // Steady-state reuse: acquire each frame; the texture must never be trimmed.
    std::shared_ptr<DirectX12Texture> reused;
    for (uint64_t frame = 2; frame < 2 + TransientTexturePool::kMaxIdleFrames * 3; ++frame)
    {
        reused = pool.Acquire(MakeDesc(64, 64), "A");
        EXPECT_EQ(reused, first);
        pool.RetireFrame(frame);
        pool.BeginFrame(frame);
    }

    EXPECT_EQ(factory.callCount, 1);
}

TEST(TransientTexturePoolTests, RetireFrame_MultipleSubmitsPerFrame_StampSeparateFences)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    pool.Acquire(MakeDesc(64, 64), "Scene");
    pool.RetireFrame(10);

    pool.Acquire(MakeDesc(32, 32), "Editor");
    pool.RetireFrame(11);

    EXPECT_EQ(pool.GetInFlightCount(), 2u);

    pool.BeginFrame(10);
    EXPECT_EQ(pool.GetFreeCount(), 1u);
    EXPECT_EQ(pool.GetInFlightCount(), 1u);

    pool.BeginFrame(11);
    EXPECT_EQ(pool.GetFreeCount(), 2u);
    EXPECT_EQ(pool.GetInFlightCount(), 0u);
}

TEST(TransientTexturePoolTests, Clear_DropsAllEntries)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    pool.Acquire(MakeDesc(64, 64), "A");
    pool.RetireFrame(1);
    pool.Acquire(MakeDesc(32, 32), "B");
    pool.BeginFrame(1);

    pool.Clear();
    EXPECT_EQ(pool.GetTotalCount(), 0u);
}

TEST(TransientTexturePoolTests, ResizeScenario_OldSizeTrimmedNewSizeReused)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    // Simulate a two-frame GPU lag between submission and completion.
    uint64_t fence = 0;
    auto frame = [&](UINT width, UINT height)
    {
        ++fence;
        pool.BeginFrame(fence >= 2 ? fence - 2 : 0);
        auto texture = pool.Acquire(MakeDesc(width, height), "Resolved");
        pool.RetireFrame(fence);
        return texture;
    };

    frame(64, 64);
    frame(64, 64);
    EXPECT_EQ(factory.callCount, 2); // frame 1's texture still in flight at frame 2

    // Resize: new dimensions allocate fresh, old-size entries age out.
    for (uint32_t i = 0; i < TransientTexturePool::kMaxIdleFrames + 6; ++i)
        frame(128, 128);

    EXPECT_EQ(pool.GetTotalCount(), 2u); // only the two 128x128 textures remain
}

TEST(RenderGraphTransientTests, CreateTexture_RegistersTransientHandleBackedByPool)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    RenderGraph graph;
    graph.SetTransientPool(&pool);

    const RenderGraphTextureHandle handle = graph.CreateTexture("ResolvedScene", MakeDesc(64, 64),
        RenderGraphTextureUsage::ShaderResource);

    ASSERT_TRUE(handle.IsValid());
    EXPECT_EQ(graph.FindImportedTexture("ResolvedScene"), handle);

    const RenderGraphTexture& texture = graph.GetImportedTexture(handle);
    EXPECT_TRUE(texture.transient);
    EXPECT_NE(texture.texture, nullptr);
    EXPECT_EQ(texture.usage, RenderGraphTextureUsage::ShaderResource);
    EXPECT_EQ(pool.GetActiveCount(), 1u);
}

TEST(RenderGraphTransientTests, ImportTexture_IsNotTransient)
{
    RenderGraph graph;
    const RenderGraphTextureHandle handle = graph.ImportTexture("SceneColor", MakeFakeTexture(),
        RenderGraphTextureUsage::ColorAttachment);

    EXPECT_FALSE(graph.GetImportedTexture(handle).transient);
}

TEST(RenderGraphTransientTests, Reset_DropsGraphReferenceButPoolRetainsTexture)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    RenderGraph graph;
    graph.SetTransientPool(&pool);

    graph.CreateTexture("ResolvedScene", MakeDesc(64, 64), RenderGraphTextureUsage::ShaderResource);
    graph.Reset();

    EXPECT_EQ(graph.GetImportedTextureCount(), 0u);
    EXPECT_EQ(pool.GetActiveCount(), 1u);

    // Next frame reuses the same texture after the fence completes.
    pool.RetireFrame(1);
    pool.BeginFrame(1);
    graph.CreateTexture("ResolvedScene", MakeDesc(64, 64), RenderGraphTextureUsage::ShaderResource);
    EXPECT_EQ(factory.callCount, 1);
}

TEST(RenderGraphTransientTests, GBufferTransientLifetime_FourFormatsPerFrame_ResizeReacquires)
{
    CountingFactory factory;
    TransientTexturePool pool;
    pool.SetTextureFactory(factory.Bind());

    RenderGraph graph;
    graph.SetTransientPool(&pool);

    const RenderGraphTextureUsage gbufferUsage =
        RenderGraphTextureUsage::ColorAttachment | RenderGraphTextureUsage::ShaderResource;

    uint64_t fence = 0;
    auto runFrame = [&](UINT width, UINT height, bool verifyTransient)
    {
        ++fence;
        pool.BeginFrame(fence >= 2 ? fence - 2 : 0);

        // Three unique pool keys per frame (albedo and material share R8G8B8A8_UNORM in production).
        const RenderGraphTextureHandle albedoHandle = graph.CreateTexture(
            "GBufferAlbedo", MakeGBufferDesc(width, height, DXGI_FORMAT_R8G8B8A8_UNORM), gbufferUsage);
        const RenderGraphTextureHandle normalHandle = graph.CreateTexture(
            "GBufferNormal", MakeGBufferDesc(width, height, DXGI_FORMAT_R16G16B16A16_FLOAT), gbufferUsage);
        const RenderGraphTextureHandle emissiveHandle = graph.CreateTexture(
            "GBufferEmissive", MakeGBufferDesc(width, height, DXGI_FORMAT_R11G11B10_FLOAT), gbufferUsage);

        if (verifyTransient)
        {
            EXPECT_TRUE(graph.GetImportedTexture(albedoHandle).transient);
            EXPECT_TRUE(graph.GetImportedTexture(normalHandle).transient);
            EXPECT_TRUE(graph.GetImportedTexture(emissiveHandle).transient);
        }

        graph.Reset();
        pool.RetireFrame(fence);
    };

    runFrame(1920, 1080, false);
    runFrame(1920, 1080, false);
    const int callsAfterWarmup = factory.callCount;
    EXPECT_EQ(callsAfterWarmup, 6);

    runFrame(1920, 1080, true);
    EXPECT_EQ(factory.callCount, callsAfterWarmup);

    const int callsBeforeResize = factory.callCount;
    for (uint32_t i = 0; i < TransientTexturePool::kMaxIdleFrames + 6; ++i)
        runFrame(1280, 720, false);

    EXPECT_EQ(factory.callCount, callsBeforeResize + 3);
    EXPECT_EQ(pool.GetTotalCount(), 6u);
}
