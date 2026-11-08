#include "Editor/EditorRenderSettings.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(EditorRenderSettingsTests, TemporalAA_DefaultsOffAndToggles)
{
    EditorRenderSettings settings;
    EXPECT_FALSE(settings.IsTemporalAAEnabled());

    settings.SetTemporalAAEnabled(true);
    EXPECT_TRUE(settings.IsTemporalAAEnabled());

    settings.SetTemporalAAEnabled(false);
    EXPECT_FALSE(settings.IsTemporalAAEnabled());
}

TEST(EditorRenderSettingsTests, TemporalSharpness_ClampedToUnitRange)
{
    EditorRenderSettings settings;
    settings.SetTemporalSharpness(-1.f);
    EXPECT_FLOAT_EQ(settings.GetTemporalSharpness(), 0.f);
    settings.SetTemporalSharpness(0.3f);
    EXPECT_FLOAT_EQ(settings.GetTemporalSharpness(), 0.3f);
    settings.SetTemporalSharpness(2.f);
    EXPECT_FLOAT_EQ(settings.GetTemporalSharpness(), 1.f);
}

TEST(EditorRenderSettingsTests, HistoryResetRequest_ConsumedOnce)
{
    EditorRenderSettings settings;
    EXPECT_FALSE(settings.ConsumeTemporalHistoryResetRequest());

    settings.RequestTemporalHistoryReset();
    EXPECT_TRUE(settings.IsTemporalHistoryResetPending());
    EXPECT_TRUE(settings.ConsumeTemporalHistoryResetRequest());
    EXPECT_FALSE(settings.ConsumeTemporalHistoryResetRequest());
}
