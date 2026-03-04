#include "Style/EditorTheme.h"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string.h>

#include "Runtime/IO/IOManager.h"

using namespace DeltaEngine;

EditorTheme::EditorTheme()
    : m_regularFont(nullptr)
    , m_boldFont(nullptr)
    , m_monoFont(nullptr)
{
}

EditorTheme::~EditorTheme()
{
}

void EditorTheme::ApplyTheme()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 6.f;
    s.ChildRounding = 6.f;
    s.FrameRounding = 4.f;
    s.PopupRounding = 6.f;
    s.ScrollbarRounding = 3.f;
    s.GrabRounding = 3.f;
    s.TabRounding = 4.f;
    s.WindowBorderSize = 1.f;
    s.FrameBorderSize = 1.f;
    s.ItemSpacing = ImVec2(4.f, 3.f);
    s.FramePadding = ImVec2(7.f, 3.f);
    s.WindowPadding = ImVec2(7.f, 5.f);
    s.ScrollbarSize = 4.f;
    s.GrabMinSize = 4.f;
    s.IndentSpacing = 14.f;

    ImVec4* colors = s.Colors;

    // Depth / Background
    colors[ImGuiCol_WindowBg] = HexToVec4(0x0d1019);
    colors[ImGuiCol_ChildBg] = HexToVec4(0x090c14);
    colors[ImGuiCol_PopupBg] = HexToVec4(0x121620);
    colors[ImGuiCol_FrameBg] = HexToVec4(0x171c28);
    colors[ImGuiCol_FrameBgHovered] = HexToVec4(0x1c2234);
    colors[ImGuiCol_FrameBgActive] = HexToVec4(0x1e2840);
    colors[ImGuiCol_TitleBg] = HexToVec4(0x0d1019);
    colors[ImGuiCol_TitleBgActive] = HexToVec4(0x121620);
    colors[ImGuiCol_TitleBgCollapsed] = HexToVec4(0x090c14);
    colors[ImGuiCol_MenuBarBg] = HexToVec4(0x090c14);

    // Borders
    colors[ImGuiCol_Border] = HexToVec4(0x191f30);
    colors[ImGuiCol_BorderShadow] = HexToVec4(0x0a0d16, 0.f);

    // Text
    colors[ImGuiCol_Text] = HexToVec4(0xbcc4de);
    colors[ImGuiCol_TextDisabled] = HexToVec4(0x6c7898);

    // Accent — Periwinkle Blue #6B8CFF
    colors[ImGuiCol_CheckMark] = HexToVec4(0x6B8CFF);
    colors[ImGuiCol_SliderGrab] = HexToVec4(0x6B8CFF);
    colors[ImGuiCol_SliderGrabActive] = HexToVec4(0x8FAAFF);
    colors[ImGuiCol_Button] = HexToVec4(0x6B8CFF, 0.05f);
    colors[ImGuiCol_ButtonHovered] = HexToVec4(0x1c2234);
    colors[ImGuiCol_ButtonActive] = HexToVec4(0x3A52B8);
    colors[ImGuiCol_Header] = HexToVec4(0x6B8CFF, 0.05f);
    colors[ImGuiCol_HeaderHovered] = HexToVec4(0x1c2234);
    colors[ImGuiCol_HeaderActive] = HexToVec4(0x3A52B8);
    colors[ImGuiCol_SeparatorHovered] = HexToVec4(0x6B8CFF);
    colors[ImGuiCol_SeparatorActive] = HexToVec4(0x8FAAFF);
    colors[ImGuiCol_ResizeGrip] = HexToVec4(0x6B8CFF, 0.15f);
    colors[ImGuiCol_ResizeGripHovered] = HexToVec4(0x6B8CFF, 0.4f);
    colors[ImGuiCol_ResizeGripActive] = HexToVec4(0x6B8CFF);
    colors[ImGuiCol_DockingPreview] = HexToVec4(0x6B8CFF, 0.4f);
    colors[ImGuiCol_DockingEmptyBg] = HexToVec4(0x060709);

    // Tabs
    colors[ImGuiCol_Tab] = HexToVec4(0x121620);
    colors[ImGuiCol_TabHovered] = HexToVec4(0x1c2234);
    colors[ImGuiCol_TabSelected] = HexToVec4(0x6B8CFF, 0.05f);
    colors[ImGuiCol_TabSelectedOverline] = HexToVec4(0x6B8CFF);
    colors[ImGuiCol_TabDimmed] = HexToVec4(0x090c14);
    colors[ImGuiCol_TabDimmedSelected] = HexToVec4(0x121620);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = HexToVec4(0x090c14);
    colors[ImGuiCol_ScrollbarGrab] = HexToVec4(0x191f30);
    colors[ImGuiCol_ScrollbarGrabHovered] = HexToVec4(0x2e3d5e);
    colors[ImGuiCol_ScrollbarGrabActive] = HexToVec4(0x6B8CFF);

    // Separator / Misc
    colors[ImGuiCol_Separator] = HexToVec4(0x191f30);
    colors[ImGuiCol_TableHeaderBg] = HexToVec4(0x121620);
    colors[ImGuiCol_TableBorderStrong] = HexToVec4(0x191f30);
    colors[ImGuiCol_TableBorderLight] = HexToVec4(0x0a0d16);

    // Named theme colors for chrome panels and UI components
    this->colors.DFloor = HexToVec4(0x090c14);
    this->colors.DRaised = HexToVec4(0x121620);
    this->colors.DHover = HexToVec4(0x1c2234);
    this->colors.AccBg = HexToVec4(0x6B8CFF, 0.05f);
    this->colors.AccMid = HexToVec4(0x6B8CFF, 0.40f);
    this->colors.AccHi = HexToVec4(0x8FAAFF);
    this->colors.BDeep = HexToVec4(0x0a0d16);
    this->colors.BMid = HexToVec4(0x191f30);
    this->colors.BLight = HexToVec4(0x222d44);
    this->colors.BHover = HexToVec4(0x2e3d5e);
    this->colors.TPrimary = HexToVec4(0xbcc4de);
    this->colors.TLabel = HexToVec4(0x6c7898);
    this->colors.TDim = HexToVec4(0x384060);
    this->colors.TGhost = HexToVec4(0x1e2640);
    this->colors.Ok = HexToVec4(0x34d399);
    this->colors.CMesh = HexToVec4(0x1ec8b4);
    this->colors.DPanel = HexToVec4(0x0d1019);

    LoadFonts();
}

void EditorTheme::LoadFonts()
{
    std::string regularFontPath = IOManager::GetEditorSourceAssetFullPath("Fonts/Outfit-Regular.ttf");
    m_regularFont = TryLoadFont(regularFontPath);

    std::string boldFontPath = IOManager::GetEditorSourceAssetFullPath("Fonts/Outfit-Bold.ttf");
    m_boldFont = TryLoadFont(boldFontPath);

    std::string monoFontPath = IOManager::GetEditorSourceAssetFullPath("Fonts/JetBrainsMono-Regular.ttf");
    m_monoFont = TryLoadFont(monoFontPath);
}

ImFont* EditorTheme::TryLoadFont(std::string path)
{
    ImFontConfig config;
    config.Flags = ImFontFlags_NoLoadError;
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(path.c_str(), 0.0f, &config);
    if (font)
    {
        return font;
    }

    std::cerr << std::format("[EditorTheme] Warning: could not load font at {}\n", path);
    return nullptr;
}

ImVec4 EditorTheme::HexToVec4(uint32_t hex, float alphaOverride) const
{
    float r, g, b, a;
    if ((hex >> 24) != 0)
    {
        // 8-digit: #RRGGBBAA
        r = ((hex >> 24) & 0xFF) / 255.f;
        g = ((hex >> 16) & 0xFF) / 255.f;
        b = ((hex >> 8) & 0xFF) / 255.f;
        a = (alphaOverride >= 0.f) ? alphaOverride : ((hex & 0xFF) / 255.f);
    }
    else
    {
        // 6-digit: #RRGGBB
        r = ((hex >> 16) & 0xFF) / 255.f;
        g = ((hex >> 8) & 0xFF) / 255.f;
        b = (hex & 0xFF) / 255.f;
        a = (alphaOverride >= 0.f) ? alphaOverride : 1.f;
    }

    return ImVec4(r, g, b, a);
}
