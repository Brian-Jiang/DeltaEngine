#include "Panels/MainToolbar.h"

#include "EditorMain.h"
#include "Style/EditorTheme.h"

#include "imgui.h"

using namespace DeltaEngine;

namespace
{
    static const char* kCoordSpaceItems[] = { "World", "Local" };
    static const char* kPivotItems[]      = { "Pivot", "Center" };
    static const char* kProjItems[]       = { "Perspective", "Orthographic" };
    static const char* kShadingItems[]    = { "Lit", "Unlit", "Wireframe" };

    // Vertical 1px divider centered on the current item row.
    // Uses GetFrameHeight() directly so it picks up the toolbar's inflated padding.
    void DrawVerticalDivider(float height = 0.f)
    {
        const float fh  = ImGui::GetFrameHeight();
        const float fs  = ImGui::GetFontSize();
        const float pad = ImGui::GetStyle().ItemSpacing.x;
        if (height <= 0.f) height = fs;
        EditorTheme* theme = g_editor->GetEditorTheme();
        const auto& c = theme->colors;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float yCenter = pos.y + fh * 0.5f;
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(pos.x, yCenter - height * 0.5f),
            ImVec2(pos.x, yCenter + height * 0.5f),
            ImGui::ColorConvertFloat4ToU32(c.BMid), 1.f);
        ImGui::SameLine(0, pad);
    }

    // Styled BeginCombo dropdown for the toolbar.
    // Call SetNextItemWidth before this to control the combo width.
    void DrawTbDropdown(const char* id, const char* const* items, int count, int& selected)
    {
        EditorTheme* theme = g_editor->GetEditorTheme();
        const auto& c = theme->colors;

        ImGui::PushStyleColor(ImGuiCol_FrameBg,          c.DRaised);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,   c.DHover);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive,    c.DHover);
        ImGui::PushStyleColor(ImGuiCol_Text,              c.TPrimary);
        ImGui::PushStyleColor(ImGuiCol_Border,            c.BLight);
        ImGui::PushStyleColor(ImGuiCol_PopupBg,           c.DFloor);
        ImGui::PushStyleColor(ImGuiCol_Header,            c.AccBg);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered,     c.DHover);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   5.f);
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding,   5.f);

        if (selected < 0 || selected >= count) selected = 0;
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
    ImGuiIO& io = ImGui::GetIO();
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    // fs is independent of FramePadding — compute before any push
    const float fs = ImGui::GetFontSize();

    ImGui::SetNextWindowPos(ImVec2(0, EditorTheme::HdrH()));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, EditorTheme::TbH()));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, c.DFloor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##MainToolbar", nullptr, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    // Push extra vertical frame padding so toolbar items are visibly larger
    // than the global default.  All fh-derived sizes below reflect this.
    {
        const auto& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
            ImVec2(style.FramePadding.x, style.FramePadding.y + fs * 0.25f));
    }

    //const float kItemH = fs * 1.55f;   // tweak this single knob
    const float fh  = ImGui::GetFrameHeight();   // inflated
    //const float fh = fs + g.Style.FramePadding.y * 2.0f;
    const float pad = ImGui::GetStyle().ItemSpacing.x;

    //ImGui::SetCursorPosY((EditorTheme::TbH() - fh) * 0.5f);
    //ImGui::SetCursorPosX(pad);

    // ── Transform toggle ────────────────────────────────────────────────────
    static const HorizontalToggleGroup::Item transformItems[] = {
        {"\xe2\x9c\xa5", "Select"},   // ✥
        {"\xe2\x8a\x95", "Move"},     // ⊕
        {"\xe2\x86\xbb", "Rotate"},   // ↻
        {"\xe2\xa4\xa2", "Scale"},    // ⤢
    };
    m_transformGroup.Draw("##xform", transformItems, 4, m_transformMode, 0.f, 0.f);

    ImGui::SameLine(0, pad);
    DrawVerticalDivider();

    // ── Coord-space dropdown ─────────────────────────────────────────────────
    ImGui::SetNextItemWidth(fs * 6.5f);
    DrawTbDropdown("##space", kCoordSpaceItems, 2, m_coordSpace);
    ImGui::SameLine(0, pad * 0.5f);

    // ── Pivot dropdown ───────────────────────────────────────────────────────
    ImGui::SetNextItemWidth(fs * 6.5f);
    DrawTbDropdown("##pivot", kPivotItems, 2, m_pivotMode);
    ImGui::SameLine(0, pad);
    DrawVerticalDivider();

    // ── Snap widget — icon | value ───────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_ChildBg, c.DRaised);
    ImGui::PushStyleColor(ImGuiCol_Border,  c.BLight);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding,    5.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize,  1.f);
    ImGui::BeginChild("##SnapWidget", ImVec2(fh * 2.5f, fh), ImGuiChildFlags_None,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  c.DHover);
    ImGui::PushStyleColor(ImGuiCol_Text,           c.AccHi);
    ImGui::Button("\xe2\x8a\x9e##SnapToggle", ImVec2(fh, fh));
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle snap");

    ImGui::SameLine(0, 0);
    if (theme->GetMonoFont())
        ImGui::PushFont(theme->GetMonoFont());
    ImGui::PushStyleColor(ImGuiCol_Text, c.CMesh);
    //ImGui::SetCursorPos(ImVec2(fh, (fh - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::Text("%.2f", m_snapValue);
    ImGui::PopStyleColor();
    if (theme->GetMonoFont())
        ImGui::PopFont();

    ImGui::EndChild();

    ImGui::SameLine(0, pad);
    DrawVerticalDivider();

    // ── Projection dropdown ──────────────────────────────────────────────────
    ImGui::SetNextItemWidth(fs * 9.5f);
    DrawTbDropdown("##proj", kProjItems, 2, m_projMode);
    ImGui::SameLine(0, pad * 0.5f);

    // ── Shading dropdown ─────────────────────────────────────────────────────
    ImGui::SetNextItemWidth(fs * 6.5f);
    DrawTbDropdown("##shading", kShadingItems, 3, m_shadingMode);
    ImGui::SameLine(0, pad);
    DrawVerticalDivider();

    // ── Play toggle ──────────────────────────────────────────────────────────
    static const HorizontalToggleGroup::Item playItems[] = {
        {"\xe2\x96\xb6", "Play"},   // ▶
        {"\xe2\x8f\xb8", "Pause"},  // ⏸
        {"\xe2\x8f\xb9", "Stop"},   // ⏹
        {"\xe2\x8f\xad", "Step"},   // ⏭
    };
    static const int playOverrideIndex = 1;
    m_playGroup.Draw("##play", playItems, 4, m_playState, 0.f, 0.f,
        (m_playState == 1) ? &playOverrideIndex : nullptr,
        (m_playState == 1) ? &c.Ok : nullptr);

    ImGui::SameLine(0, pad);
    DrawVerticalDivider();

    // ── FPS label ────────────────────────────────────────────────────────────
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

    ImGui::PopStyleVar();   // extra FramePadding
    ImGui::End();
}
