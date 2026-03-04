#include "Panels/MainToolbar.h"

#include "EditorMain.h"
#include "Style/EditorTheme.h"

#include "imgui.h"

using namespace DeltaEngine;

namespace
{
    void DrawVerticalDivider(float height = 20.f)
    {
        EditorTheme* theme = g_editor->GetEditorTheme();
        const auto& c = theme->colors;
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float yCenter = pos.y + ImGui::GetTextLineHeight() * 0.5f;
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(pos.x, yCenter - height * 0.5f),
            ImVec2(pos.x, yCenter + height * 0.5f),
            ImGui::ColorConvertFloat4ToU32(c.BMid), 1.f);
        ImGui::SameLine(0, 8);
    }

    void DrawTbDropButton(const char* label, float height = 28.f)
    {
        EditorTheme* theme = g_editor->GetEditorTheme();
        const auto& c = theme->colors;
        ImGui::PushStyleColor(ImGuiCol_Button, c.DRaised);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c.DHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, c.DHover);
        ImGui::PushStyleColor(ImGuiCol_Text, c.TPrimary);
        ImGui::PushStyleColor(ImGuiCol_Border, c.BLight);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
        ImGui::Button(label, ImVec2(0, height));
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);
    }
}

void MainToolbar::Draw()
{
    ImGuiIO& io = ImGui::GetIO();
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    ImGui::SetNextWindowPos(ImVec2(0, EditorTheme::kHdrH));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, EditorTheme::kTbH));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, c.DFloor);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##MainToolbar", nullptr, flags);
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY((EditorTheme::kTbH - 24.f) * 0.5f);
    ImGui::SetCursorPosX(12);

    // Transform toggle
    static const HorizontalToggleGroup::Item transformItems[] = {
        {"\xe2\x9c\xa5", "Select"},   // ✥
        {"\xe2\x8a\x95", "Move"},     // ⊕
        {"\xe2\x86\xbb", "Rotate"},    // ↻
        {"\xe2\xa4\xa2", "Scale"},    // ⤢
    };
    m_transformGroup.Draw("##xform", transformItems, 4, m_transformMode, 28.f, 24.f);

    ImGui::SameLine(0, 8);
    DrawVerticalDivider(20.f);

    // Coord space dropdown (placeholder)
    DrawTbDropButton("World \xe2\x96\xbe");
    ImGui::SameLine(0, 4);

    // Pivot dropdown (placeholder)
    DrawTbDropButton("Pivot \xe2\x96\xbe");
    ImGui::SameLine(0, 8);
    DrawVerticalDivider(20.f);

    // Snap widget — two-cell row: icon | value
    ImGui::PushStyleColor(ImGuiCol_ChildBg, c.DRaised);
    ImGui::PushStyleColor(ImGuiCol_Border, c.BLight);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.f);
    ImGui::BeginChild("##SnapWidget", ImVec2(60, 28), ImGuiChildFlags_None,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    // Left cell: snap icon
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, c.DHover);
    ImGui::PushStyleColor(ImGuiCol_Text, c.AccHi);
    ImGui::Button("\xe2\x8a\x9e##SnapToggle", ImVec2(28, 28));
    ImGui::PopStyleColor(3);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle snap");

    // Right cell: snap value
    ImGui::SameLine(0, 0);
    if (theme->GetMonoFont())
        ImGui::PushFont(theme->GetMonoFont());
    ImGui::PushStyleColor(ImGuiCol_Text, c.CMesh);
    ImGui::SetCursorPos(ImVec2(28, (28 - ImGui::GetTextLineHeight()) * 0.5f));
    ImGui::Text("%.2f", m_snapValue);
    ImGui::PopStyleColor();
    if (theme->GetMonoFont())
        ImGui::PopFont();

    ImGui::EndChild();

    ImGui::SameLine(0, 8);
    DrawVerticalDivider(20.f);

    // Perspective dropdown (placeholder)
    DrawTbDropButton("Perspective \xe2\x96\xbe");
    ImGui::SameLine(0, 4);

    // Lit dropdown (placeholder)
    DrawTbDropButton("Lit \xe2\x96\xbe");
    ImGui::SameLine(0, 8);
    DrawVerticalDivider(20.f);

    // Play toggle — when playing (index 1), use Ok green for selected state
    static const HorizontalToggleGroup::Item playItems[] = {
        {"\xe2\x96\xb6", "Play"},   // ▶
        {"\xe2\x8f\xb8", "Pause"},  // ⏸
        {"\xe2\x8f\xb9", "Stop"},   // ⏹
        {"\xe2\x8f\xad", "Step"},   // ⏭
    };
    static const int playOverrideIndex = 1;
    m_playGroup.Draw("##play", playItems, 4, m_playState, 27.f, 24.f,
        (m_playState == 1) ? &playOverrideIndex : nullptr,
        (m_playState == 1) ? &c.Ok : nullptr);

    ImGui::SameLine(0, 8);
    DrawVerticalDivider(20.f);

    // FPS label — frame with bg and border, then text
    ImVec2 fpsCursor = ImGui::GetCursorScreenPos();
    float fpsW = 60.f;
    float fpsH = 28.f;
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
        fpsCursor.y + (fpsH - fpsTextSize.y) * 0.5f));
    ImGui::Text("%s", fpsBuf);
    if (theme->GetMonoFont())
        ImGui::PopFont();
    //ImGui::SetCursorScreenPos(ImVec2(fpsCursor.x + fpsW + 8, fpsCursor.y));

    ImGui::End();
}
