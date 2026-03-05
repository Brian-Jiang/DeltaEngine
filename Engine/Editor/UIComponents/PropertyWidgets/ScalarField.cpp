#include "UIComponents/PropertyWidgets/ScalarField.h"
#include "UIComponents/PropertyWidgets/PropertyWidgetUtil.h"

#include "EditorMain.h"
#include "Style/EditorTheme.h"
#include "imgui.h"

using namespace DeltaEngine;

bool ScalarField::Draw(const char* label, float* value, float speed, const char* fmt)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;
    ImFont*      mono  = theme->GetMonoFont();

    ImGui::PushID(label);

    float availW = BeginPropertyRow(label, c);
    ImGui::SetNextItemWidth(availW);

    ImGui::PushStyleColor(ImGuiCol_FrameBg,        c.DInput);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, c.DHover);
    ImGui::PushStyleColor(ImGuiCol_Border,         c.BLight);
    ImGui::PushStyleColor(ImGuiCol_Text,           c.TPrimary);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   3.f);

    if (mono) ImGui::PushFont(mono);
    bool changed = ImGui::DragFloat("##v", value, speed, 0.f, 0.f, fmt);
    if (mono) ImGui::PopFont();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    EndPropertyRow();
    ImGui::PopID();

    return changed;
}
