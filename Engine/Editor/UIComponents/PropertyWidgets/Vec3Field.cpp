#include "UIComponents/PropertyWidgets/Vec3Field.h"
#include "UIComponents/PropertyWidgets/PropertyWidgetUtil.h"

#include "EditorMain.h"
#include "Style/EditorTheme.h"
#include "imgui.h"

using namespace DeltaEngine;

bool Vec3Field::Draw(const char* label, float* values, float speed, const char* fmt)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;
    ImFont*      mono  = theme->GetMonoFont();
    ImDrawList*  dl    = ImGui::GetWindowDrawList();

    ImGui::PushID(label);

    float availW = BeginPropertyRow(label, c);

    const float fs     = ImGui::GetFontSize();
    const float fh     = ImGui::GetFrameHeight();
    const float chipW  = fs * 1.4f;
    const float gap    = 2.f;
    const float fieldW = (availW - 2.f * gap) / 3.f;

    static const char* kAxisLabels[3] = { "X", "Y", "Z" };
    bool changed = false;

    for (int i = 0; i < 3; ++i)
    {
        ImGui::PushID(i);
        ImGui::BeginGroup();

        ImVec2 pos = ImGui::GetCursorScreenPos();

        // Axis chip colors
        ImVec4 axBg  = (i == 0) ? c.AxXBg : (i == 1) ? c.AxYBg : c.AxZBg;
        ImVec4 axCol = (i == 0) ? c.AxX   : (i == 1) ? c.AxY   : c.AxZ;

        // Draw chip background (left-side rounded)
        dl->AddRectFilled(
            pos,
            ImVec2(pos.x + chipW, pos.y + fh),
            ImGui::ColorConvertFloat4ToU32(axBg),
            4.f, ImDrawFlags_RoundCornersLeft);

        // Draw axis letter centered in chip
        const char* axLabel = kAxisLabels[i];
        ImVec2 textSz = ImGui::CalcTextSize(axLabel);
        dl->AddText(
            ImVec2(pos.x + (chipW - textSz.x) * 0.5f,
                   pos.y + (fh   - textSz.y) * 0.5f),
            ImGui::ColorConvertFloat4ToU32(axCol),
            axLabel);

        // Advance cursor past chip without rendering a visible widget
        ImGui::Dummy(ImVec2(chipW, fh));
        ImGui::SameLine(0.f, 0.f);

        // DragFloat for this component
        float dragW = fieldW - chipW;
        ImGui::SetNextItemWidth(dragW);

        ImGui::PushStyleColor(ImGuiCol_FrameBg,        c.DInput);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, c.DHover);
        ImGui::PushStyleColor(ImGuiCol_Border,         c.BLight);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   3.f);

        if (mono) ImGui::PushFont(mono);
        changed |= ImGui::DragFloat("##v", &values[i], speed, 0.f, 0.f, fmt);
        if (mono) ImGui::PopFont();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);

        ImGui::EndGroup();

        if (i < 2)
            ImGui::SameLine(0.f, gap);

        ImGui::PopID();
    }

    EndPropertyRow();
    ImGui::PopID();

    return changed;
}
