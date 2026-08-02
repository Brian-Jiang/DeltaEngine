#include "Panels/MainToolbar.h"

#include "EditorCore.h"
#include "EditorMain.h"
#include "Panels/EditorChromeContext.h"
#include "Style/EditorTheme.h"

#include "imgui.h"

using namespace DeltaEngine;

namespace
{
const char* kCoordSpaceItems[] = { "World", "Local" };
const char* kPivotItems[]      = { "Pivot", "Center" };
const char* kProjItems[]       = { "Perspective", "Orthographic" };
const char* kShadingItems[]    = { "Lit", "Unlit", "Wireframe" };

void DrawVerticalDivider(const EditorTheme::ThemeColors& c, float height = 0.f)
{
    const float fh  = ImGui::GetFrameHeight();
    const float fs  = ImGui::GetFontSize();
    const float pad = ImGui::GetStyle().ItemSpacing.x;
    if (height <= 0.f)
        height = fs;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float yCenter = pos.y + fh * 0.5f;
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(pos.x, yCenter - height * 0.5f),
        ImVec2(pos.x, yCenter + height * 0.5f),
        ImGui::ColorConvertFloat4ToU32(c.BMid), 1.f);
    ImGui::SameLine(0, pad);
}

void DrawTbDropdown(const char* id, const EditorTheme::ThemeColors& c, const char* const* items, int count,
    int& selected)
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, c.DRaised);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, c.DHover);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, c.DHover);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TPrimary);
    ImGui::PushStyleColor(ImGuiCol_Border, c.BLight);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, c.DFloor);
    ImGui::PushStyleColor(ImGuiCol_Header, c.AccBg);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, c.DHover);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 5.f);

    if (selected < 0 || selected >= count)
        selected = 0;
    if (ImGui::BeginCombo(id, items[selected]))
    {
        for (int i = 0; i < count; ++i)
        {
            bool isSelected = (selected == i);
            if (ImGui::Selectable(items[i], isSelected))
                selected = i;
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(8);
}
}

void MainToolbar::Draw()
{
    EditorChromeContext ctx;
    ctx.theme  = g_editor ? g_editor->GetEditorTheme() : nullptr;
    ctx.core   = g_editorCore;
    ctx.editor = g_editor;
    Draw(ctx);
}

void MainToolbar::Draw(const EditorChromeContext& ctx)
{
    if (!ctx.theme)
    {
        DLOG(LogEditorChrome, ELogLevel::Warning,
            "MainToolbar::Draw skipped: missing EditorTheme (expected non-null ctx.theme; core={}, editor={})",
            static_cast<const void*>(ctx.core), static_cast<const void*>(ctx.editor));
        return;
    }

    EditorTheme* theme = ctx.theme;
    DELTA_ASSERT(theme != nullptr);
    const auto& c = theme->colors;

    ImGuiIO& io = ImGui::GetIO();

    const float fs = ImGui::GetFontSize();

    ImGui::SetNextWindowPos(ImVec2(0, EditorTheme::HdrH()));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, EditorTheme::TbH()));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, c.DFloor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##MainToolbar", nullptr, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    {
        const auto& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
            ImVec2(style.FramePadding.x, style.FramePadding.y + fs * 0.25f));
    }

    const float fh  = ImGui::GetFrameHeight();
    const float pad = ImGui::GetStyle().ItemSpacing.x;

    static const HorizontalToggleGroup::Item transformItems[] = {
        {"\xef\x89\x96", "Select"},
        {"\xef\x82\xb2", "Move"},
        {"\xef\x8b\xb9", "Rotate"},
        {"\xef\x90\xa4", "Scale"},
    };
    m_transformGroup.Draw("##xform", c, transformItems, 4, m_transformMode, 0.f, 0.f);

    ImGui::SameLine(0, pad);
    DrawVerticalDivider(c);

    ImGui::SetNextItemWidth(fs * 6.5f);
    DrawTbDropdown("##space", c, kCoordSpaceItems, 2, m_coordSpace);
    ImGui::SameLine(0, pad * 0.5f);

    ImGui::SetNextItemWidth(fs * 6.5f);
    DrawTbDropdown("##pivot", c, kPivotItems, 2, m_pivotMode);
    ImGui::SameLine(0, pad);
    DrawVerticalDivider(c);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, c.DRaised);
    ImGui::PushStyleColor(ImGuiCol_Border, c.BLight);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
    ImGui::BeginChild("##SnapWidget", ImVec2(fh * 2.5f, fh), ImGuiChildFlags_None,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c.DHover);
    ImGui::PushStyleColor(ImGuiCol_Text, c.AccHi);
    ImGui::Button("\xef\xa1\x8c##SnapToggle", ImVec2(fh, fh));
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle snap");

    ImGui::SameLine(0, pad);
    if (theme->GetMonoFont())
        ImGui::PushFont(theme->GetMonoFont());
    ImGui::PushStyleColor(ImGuiCol_Text, c.CMesh);
    ImGui::Text("%.2f", m_snapValue);
    ImGui::PopStyleColor();
    if (theme->GetMonoFont())
        ImGui::PopFont();

    ImGui::EndChild();

    ImGui::SameLine(0, pad);
    DrawVerticalDivider(c);

    ImGui::SetNextItemWidth(fs * 9.5f);
    DrawTbDropdown("##proj", c, kProjItems, 2, m_projMode);
    ImGui::SameLine(0, pad * 0.5f);

    ImGui::SetNextItemWidth(fs * 6.5f);
    DrawTbDropdown("##shading", c, kShadingItems, 3, m_shadingMode);
    ImGui::SameLine(0, pad);
    DrawVerticalDivider(c);

    static const HorizontalToggleGroup::Item playItems[] = {
        {"\xef\x81\x8b", "Play"},
        {"\xef\x81\x8c", "Pause"},
        {"\xef\x81\x8d", "Stop"},
        {"\xef\x81\x91", "Step"},
    };
    static const int playOverrideIndex = 1;
    m_playGroup.Draw("##play", c, playItems, 4, m_playState, 0.f, 0.f,
        (m_playState == 1) ? &playOverrideIndex : nullptr,
        (m_playState == 1) ? &c.Ok : nullptr);

    ImGui::SameLine(0, pad);
    DrawVerticalDivider(c);

    ImVec2 fpsCursor = ImGui::GetCursorScreenPos();
    float fpsW = fs * 4.5f;
    float fpsH = fh;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(fpsCursor, ImVec2(fpsCursor.x + fpsW, fpsCursor.y + fpsH),
        ImGui::ColorConvertFloat4ToU32(c.DRaised), 4.f);
    dl->AddRect(fpsCursor, ImVec2(fpsCursor.x + fpsW, fpsCursor.y + fpsH),
        ImGui::ColorConvertFloat4ToU32(c.BLight), 4.f, 0, 1.f);
    if (theme->GetMonoFont())
        ImGui::PushFont(theme->GetMonoFont());
    char fpsBuf[32];
    snprintf(fpsBuf, sizeof(fpsBuf), "%.1f fps", io.Framerate);
    ImVec2 fpsTextSize = ImGui::CalcTextSize(fpsBuf);
    ImGui::SetCursorScreenPos(ImVec2(fpsCursor.x + (fpsW - fpsTextSize.x) * 0.5f,
        fpsCursor.y));
    ImGui::Text("%s", fpsBuf);
    if (theme->GetMonoFont())
        ImGui::PopFont();

    ImGui::PopStyleVar();
    ImGui::End();
}
