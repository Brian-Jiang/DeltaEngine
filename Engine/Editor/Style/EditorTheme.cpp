#include "Style/EditorTheme.h"

#include "Runtime/IO/IOManager.h"

#include "imgui.h"

#include <filesystem>

using namespace DeltaEngine;

DEFINE_LOG_CATEGORY(DeltaEngine::LogEditorTheme)

float EditorTheme::RowH()
{
    return ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
}

float EditorTheme::HdrH()
{
    const float kExtra = ImGui::GetFontSize() * 0.25f;
    return ImGui::GetFontSize() + (ImGui::GetStyle().FramePadding.y + kExtra) * 2.f;
}

float EditorTheme::TbH()
{
    const float kItemH = ImGui::GetFontSize() * 2.0f;
    return kItemH + ImGui::GetStyle().ItemSpacing.y * 2.f;
}

float EditorTheme::StH()
{
    return ImGui::GetFrameHeight();
}

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

    ImVec4* imguiColors = s.Colors;

    imguiColors[ImGuiCol_WindowBg] = HexToVec4(0x0d1019);
    imguiColors[ImGuiCol_ChildBg] = HexToVec4(0x090c14);
    imguiColors[ImGuiCol_PopupBg] = HexToVec4(0x121620);
    imguiColors[ImGuiCol_FrameBg] = HexToVec4(0x171c28);
    imguiColors[ImGuiCol_FrameBgHovered] = HexToVec4(0x1c2234);
    imguiColors[ImGuiCol_FrameBgActive] = HexToVec4(0x1e2840);
    imguiColors[ImGuiCol_TitleBg] = HexToVec4(0x0d1019);
    imguiColors[ImGuiCol_TitleBgActive] = HexToVec4(0x121620);
    imguiColors[ImGuiCol_TitleBgCollapsed] = HexToVec4(0x090c14);
    imguiColors[ImGuiCol_MenuBarBg] = HexToVec4(0x090c14);

    imguiColors[ImGuiCol_Border] = HexToVec4(0x191f30);
    imguiColors[ImGuiCol_BorderShadow] = HexToVec4(0x0a0d16, 0.f);

    imguiColors[ImGuiCol_Text] = HexToVec4(0xbcc4de);
    imguiColors[ImGuiCol_TextDisabled] = HexToVec4(0x6c7898);

    imguiColors[ImGuiCol_CheckMark] = HexToVec4(0x6B8CFF);
    imguiColors[ImGuiCol_SliderGrab] = HexToVec4(0x6B8CFF);
    imguiColors[ImGuiCol_SliderGrabActive] = HexToVec4(0x8FAAFF);
    imguiColors[ImGuiCol_Button] = HexToVec4(0x6B8CFF, 0.05f);
    imguiColors[ImGuiCol_ButtonHovered] = HexToVec4(0x1c2234);
    imguiColors[ImGuiCol_ButtonActive] = HexToVec4(0x3A52B8);
    imguiColors[ImGuiCol_Header] = HexToVec4(0x6B8CFF, 0.05f);
    imguiColors[ImGuiCol_HeaderHovered] = HexToVec4(0x1c2234);
    imguiColors[ImGuiCol_HeaderActive] = HexToVec4(0x3A52B8);
    imguiColors[ImGuiCol_SeparatorHovered] = HexToVec4(0x6B8CFF);
    imguiColors[ImGuiCol_SeparatorActive] = HexToVec4(0x8FAAFF);
    imguiColors[ImGuiCol_ResizeGrip] = HexToVec4(0x6B8CFF, 0.15f);
    imguiColors[ImGuiCol_ResizeGripHovered] = HexToVec4(0x6B8CFF, 0.4f);
    imguiColors[ImGuiCol_ResizeGripActive] = HexToVec4(0x6B8CFF);
    imguiColors[ImGuiCol_DockingPreview] = HexToVec4(0x6B8CFF, 0.4f);
    imguiColors[ImGuiCol_DockingEmptyBg] = HexToVec4(0x060709);

    imguiColors[ImGuiCol_Tab] = HexToVec4(0x121620);
    imguiColors[ImGuiCol_TabHovered] = HexToVec4(0x1c2234);
    imguiColors[ImGuiCol_TabSelected] = HexToVec4(0x6B8CFF, 0.05f);
    imguiColors[ImGuiCol_TabSelectedOverline] = HexToVec4(0x6B8CFF);
    imguiColors[ImGuiCol_TabDimmed] = HexToVec4(0x090c14);
    imguiColors[ImGuiCol_TabDimmedSelected] = HexToVec4(0x121620);

    imguiColors[ImGuiCol_ScrollbarBg] = HexToVec4(0x090c14);
    imguiColors[ImGuiCol_ScrollbarGrab] = HexToVec4(0x191f30);
    imguiColors[ImGuiCol_ScrollbarGrabHovered] = HexToVec4(0x2e3d5e);
    imguiColors[ImGuiCol_ScrollbarGrabActive] = HexToVec4(0x6B8CFF);

    imguiColors[ImGuiCol_Separator] = HexToVec4(0x191f30);
    imguiColors[ImGuiCol_TableHeaderBg] = HexToVec4(0x121620);
    imguiColors[ImGuiCol_TableBorderStrong] = HexToVec4(0x191f30);
    imguiColors[ImGuiCol_TableBorderLight] = HexToVec4(0x0a0d16);

    colors.DFloor = HexToVec4(0x090c14);
    colors.DRaised = HexToVec4(0x121620);
    colors.DHover = HexToVec4(0x1c2234);
    colors.AccBg = HexToVec4(0x6B8CFF, 0.05f);
    colors.AccMid = HexToVec4(0x6B8CFF, 0.40f);
    colors.AccHi = HexToVec4(0x8FAAFF);
    colors.BDeep = HexToVec4(0x0a0d16);
    colors.BMid = HexToVec4(0x191f30);
    colors.BLight = HexToVec4(0x222d44);
    colors.BHover = HexToVec4(0x2e3d5e);
    colors.TPrimary = HexToVec4(0xbcc4de);
    colors.TLabel = HexToVec4(0x6c7898);
    colors.TDim = HexToVec4(0x384060);
    colors.TGhost = HexToVec4(0x1e2640);
    colors.Ok = HexToVec4(0x34d399);
    colors.CMesh = HexToVec4(0x1ec8b4);
    colors.DPanel = HexToVec4(0x0d1019);
    colors.DInput = HexToVec4(0x0b0f1a);
    colors.TBright = HexToVec4(0xecf0ff);
    colors.Err = HexToVec4(0xef4444);
    colors.AxX = HexToVec4(0xf87171);
    colors.AxY = HexToVec4(0x4ade80);
    colors.AxZ = HexToVec4(0x60a5fa);
    colors.AxXBg = HexToVec4(0x3b1212);
    colors.AxYBg = HexToVec4(0x0f2e18);
    colors.AxZBg = HexToVec4(0x0d1e38);
    colors.Acc = HexToVec4(0x6B8CFF, 0.85f);
    colors.AccRim = HexToVec4(0x6B8CFF, 0.18f);
    colors.Warn = HexToVec4(0xf59e0b);

    LoadFonts();

    DLOG(LogEditorTheme, ELogLevel::Log, "Editor ImGui theme applied (fonts regular={}, bold={}, mono={})",
        static_cast<const void*>(m_regularFont), static_cast<const void*>(m_boldFont),
        static_cast<const void*>(m_monoFont));
}

void EditorTheme::LoadFonts()
{
    const std::filesystem::path regularFontPath(IOManager::GetEditorSourceAssetFullPath("Fonts/Outfit-Regular.ttf"));
    m_regularFont = TryLoadFont(regularFontPath, true);

    const std::filesystem::path boldFontPath(IOManager::GetEditorSourceAssetFullPath("Fonts/Outfit-Bold.ttf"));
    m_boldFont = TryLoadFont(boldFontPath, true);

    const std::filesystem::path monoFontPath(IOManager::GetEditorSourceAssetFullPath("Fonts/JetBrainsMono-Regular.ttf"));
    m_monoFont = TryLoadFont(monoFontPath, false);
}

ImFont* EditorTheme::TryLoadFont(const std::filesystem::path& path, bool withFaSolid)
{
    const std::string pathUtf8 = path.string();

    ImFontConfig config;
    config.Flags = ImFontFlags_NoLoadError;
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(pathUtf8.c_str(), 0.0f, &config);
    if (font)
    {
        if (withFaSolid)
        {
            const std::filesystem::path faSolidFontPath(
                IOManager::GetEditorSourceAssetFullPath("Fonts/Font Awesome 7 Free-Solid-900.otf"));
            ImFontConfig faConfig;
            faConfig.MergeMode = true;
            faConfig.PixelSnapH = true;
            const std::string faUtf8 = faSolidFontPath.string();
            ImFont* merged = ImGui::GetIO().Fonts->AddFontFromFileTTF(faUtf8.c_str(), 0.0f, &faConfig);
            if (!merged)
            {
                DLOG(LogEditorTheme, ELogLevel::Warning,
                    "Failed to merge Font Awesome into '{}': secondary font load failed (expected merge target '{}')",
                    pathUtf8, faUtf8);
            }
        }

        return font;
    }

    DLOG(LogEditorTheme, ELogLevel::Warning,
        "Failed to load editor font from '{}': file missing or ImGui AddFontFromFileTTF returned null (expected valid TTF)",
        pathUtf8);
    return nullptr;
}

ImVec4 EditorTheme::HexToVec4(uint32_t hex, float alphaOverride) const
{
    return ColorFromHex(hex, alphaOverride);
}
