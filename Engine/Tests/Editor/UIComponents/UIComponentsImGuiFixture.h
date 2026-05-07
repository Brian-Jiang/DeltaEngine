#pragma once

#include "Editor/EditorCoreFixture.h"
#include "Editor/Style/EditorTheme.h"
#include "Editor/UIComponents/UIComponentsEditorTheme.h"

#include <gtest/gtest.h>

#include "imgui.h"

namespace DeltaEngine::Tests
{
inline void PopulateTestEditorThemeColors(EditorTheme::ThemeColors& c)
{
    c.DFloor   = EditorTheme::ColorFromHex(0x090c14);
    c.DRaised  = EditorTheme::ColorFromHex(0x121620);
    c.DHover   = EditorTheme::ColorFromHex(0x1c2234);
    c.AccBg    = EditorTheme::ColorFromHex(0x6B8CFF, 0.05f);
    c.AccMid   = EditorTheme::ColorFromHex(0x6B8CFF, 0.40f);
    c.AccHi    = EditorTheme::ColorFromHex(0x8FAAFF);
    c.BDeep    = EditorTheme::ColorFromHex(0x0a0d16);
    c.BMid     = EditorTheme::ColorFromHex(0x191f30);
    c.BLight   = EditorTheme::ColorFromHex(0x222d44);
    c.BHover   = EditorTheme::ColorFromHex(0x2e3d5e);
    c.TPrimary = EditorTheme::ColorFromHex(0xbcc4de);
    c.TLabel   = EditorTheme::ColorFromHex(0x6c7898);
    c.TDim     = EditorTheme::ColorFromHex(0x384060);
    c.TGhost   = EditorTheme::ColorFromHex(0x1e2640);
    c.Ok       = EditorTheme::ColorFromHex(0x34d399);
    c.CMesh    = EditorTheme::ColorFromHex(0x1ec8b4);
    c.DPanel   = EditorTheme::ColorFromHex(0x0d1019);
    c.DInput   = EditorTheme::ColorFromHex(0x0b0f1a);
    c.TBright  = EditorTheme::ColorFromHex(0xecf0ff);
    c.Err      = EditorTheme::ColorFromHex(0xef4444);
    c.AxX      = EditorTheme::ColorFromHex(0xf87171);
    c.AxY      = EditorTheme::ColorFromHex(0x4ade80);
    c.AxZ      = EditorTheme::ColorFromHex(0x60a5fa);
    c.AxXBg    = EditorTheme::ColorFromHex(0x3b1212);
    c.AxYBg    = EditorTheme::ColorFromHex(0x0f2e18);
    c.AxZBg    = EditorTheme::ColorFromHex(0x0d1e38);
    c.Acc      = EditorTheme::ColorFromHex(0x6B8CFF, 0.85f);
    c.AccRim   = EditorTheme::ColorFromHex(0x6B8CFF, 0.18f);
    c.Warn     = EditorTheme::ColorFromHex(0xf59e0b);
}

inline void UIComponentsImGuiTestFrameBegin(EditorTheme& theme)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(1280, 720);
    ImGui::StyleColorsDark();
    unsigned char* fontTex = nullptr;
    int              fontW = 0, fontH = 0;
    io.Fonts->GetTexDataAsRGBA32(&fontTex, &fontW, &fontH);
    (void)fontTex;
    (void)fontW;
    (void)fontH;
    PopulateTestEditorThemeColors(theme.colors);
    SetUIComponentsEditorThemeForTests(&theme);
    ImGui::NewFrame();
    ImGui::Begin("UIComponentsTestWindow", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize);
}

inline void UIComponentsImGuiTestFrameEnd()
{
    ImGui::End();
    ImGui::Render();
    SetUIComponentsEditorThemeForTests(nullptr);
    ImGui::DestroyContext();
}

class UIComponentsImGuiFixture : public ::testing::Test
{
protected:
    EditorTheme m_theme{};

    void SetUp() override { UIComponentsImGuiTestFrameBegin(m_theme); }

    void TearDown() override { UIComponentsImGuiTestFrameEnd(); }
};

class UIComponentsObjectPtrFixture : public EditorCoreFixture
{
protected:
    EditorTheme m_imguiTheme{};

    void SetUp() override
    {
        EditorCoreFixture::SetUp();
        UIComponentsImGuiTestFrameBegin(m_imguiTheme);
    }

    void TearDown() override
    {
        UIComponentsImGuiTestFrameEnd();
        EditorCoreFixture::TearDown();
    }
};

}
