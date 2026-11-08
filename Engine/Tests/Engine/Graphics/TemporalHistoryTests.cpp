#include "Runtime/Graphics/TemporalHistory.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

using namespace DeltaEngine;

namespace
{
std::shared_ptr<DirectX12Texture> MakeFakeTexture()
{
    auto owner = std::make_shared<int>(0);
    return std::shared_ptr<DirectX12Texture>(owner, reinterpret_cast<DirectX12Texture*>(owner.get()));
}

struct RecordingFactory
{
    std::vector<D3D12_RESOURCE_DESC> descs;
    std::vector<std::string> names;

    TemporalHistory::TextureFactory Bind()
    {
        return [this](const D3D12_RESOURCE_DESC& desc, const std::string& name)
        {
            descs.push_back(desc);
            names.push_back(name);
            return MakeFakeTexture();
        };
    }
};
} // namespace

TEST(TemporalHistoryTests, Resize_CreatesTwoSingleSampleHistoryTextures)
{
    RecordingFactory factory;
    TemporalHistory history;
    history.SetTextureFactory(factory.Bind());

    history.Resize(320, 180);

    ASSERT_EQ(factory.descs.size(), 2u);
    for (const D3D12_RESOURCE_DESC& desc : factory.descs)
    {
        EXPECT_EQ(desc.Width, 320u);
        EXPECT_EQ(desc.Height, 180u);
        EXPECT_EQ(desc.Format, kTemporalHistoryFormat);
        EXPECT_EQ(desc.SampleDesc.Count, 1u);
        EXPECT_NE(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, 0);
    }
    EXPECT_TRUE(history.HasTextures());
    EXPECT_NE(history.GetReadTexture(), history.GetWriteTexture());
    EXPECT_EQ(history.GetWidth(), 320u);
    EXPECT_EQ(history.GetHeight(), 180u);
    EXPECT_FALSE(history.IsValid());
}

TEST(TemporalHistoryTests, Resize_RecreatesTexturesAndInvalidatesHistory)
{
    RecordingFactory factory;
    TemporalHistory history;
    history.SetTextureFactory(factory.Bind());
    history.Resize(64, 64);
    history.EndFrame(true);
    ASSERT_TRUE(history.IsValid());
    ASSERT_EQ(history.GetReadIndex(), 1u);

    const auto oldRead = history.GetReadTexture();
    const auto oldWrite = history.GetWriteTexture();

    history.Resize(128, 96);

    EXPECT_EQ(factory.descs.size(), 4u);
    EXPECT_NE(history.GetReadTexture(), oldRead);
    EXPECT_NE(history.GetReadTexture(), oldWrite);
    EXPECT_NE(history.GetWriteTexture(), oldRead);
    EXPECT_NE(history.GetWriteTexture(), oldWrite);
    EXPECT_EQ(factory.descs[2].Width, 128u);
    EXPECT_EQ(factory.descs[2].Height, 96u);
    EXPECT_FALSE(history.IsValid());
    EXPECT_EQ(history.GetReadIndex(), 0u);
}

TEST(TemporalHistoryTests, EndFrame_SwapsReadWriteAndTracksValidity)
{
    RecordingFactory factory;
    TemporalHistory history;
    history.SetTextureFactory(factory.Bind());
    history.Resize(64, 64);

    const auto first = history.GetReadTexture();
    const auto second = history.GetWriteTexture();

    history.EndFrame(true);
    EXPECT_EQ(history.GetReadTexture(), second);
    EXPECT_EQ(history.GetWriteTexture(), first);
    EXPECT_TRUE(history.IsValid());

    history.EndFrame(true);
    EXPECT_EQ(history.GetReadTexture(), first);
    EXPECT_EQ(history.GetWriteTexture(), second);
    EXPECT_TRUE(history.IsValid());
}

TEST(TemporalHistoryTests, EndFrame_NotWritten_LeavesHistoryInvalid)
{
    RecordingFactory factory;
    TemporalHistory history;
    history.SetTextureFactory(factory.Bind());
    history.Resize(64, 64);
    history.EndFrame(true);

    history.EndFrame(false);

    EXPECT_FALSE(history.IsValid());
    EXPECT_EQ(history.GetReadIndex(), 0u);
}

TEST(TemporalHistoryTests, Invalidate_ClearsValidity)
{
    RecordingFactory factory;
    TemporalHistory history;
    history.SetTextureFactory(factory.Bind());
    history.Resize(64, 64);
    history.EndFrame(true);
    const auto read = history.GetReadTexture();

    history.Invalidate();

    EXPECT_FALSE(history.IsValid());
    EXPECT_EQ(history.GetReadTexture(), read);
}

TEST(TemporalHistoryTests, Clear_DropsTextures)
{
    RecordingFactory factory;
    TemporalHistory history;
    history.SetTextureFactory(factory.Bind());
    history.Resize(64, 64);

    history.Clear();

    EXPECT_FALSE(history.HasTextures());
    EXPECT_FALSE(history.IsValid());
}
