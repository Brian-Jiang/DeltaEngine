#include "UIComponents/PropertyWidgets/StringField.h"
#include "UIComponents/PropertyWidgets/PropertyWidgetUtil.h"

#include "EditorMain.h"
#include "Style/EditorTheme.h"
#include "imgui.h"

using namespace DeltaEngine;

WidgetEditEvent StringField::Draw(const char* label, char* buf, size_t bufSize, bool readOnly)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;

    ImGui::PushID(label);

    float availW = BeginPropertyRow(label, c);
    ImGui::SetNextItemWidth(availW);

    ImGui::PushStyleColor(ImGuiCol_FrameBg,        c.DInput);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, c.DHover);
    ImGui::PushStyleColor(ImGuiCol_Border,         c.BLight);
    ImGui::PushStyleColor(ImGuiCol_Text,           readOnly ? c.TDim : c.TPrimary);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   3.f);

    ImGuiInputTextFlags inputFlags = readOnly ? ImGuiInputTextFlags_ReadOnly : 0;
    ImGui::InputText("##s", buf, bufSize, inputFlags);

    WidgetEditEvent evt;
    evt.valueChanged = ImGui::IsItemEdited();
    evt.editBegan    = ImGui::IsItemActivated();
    evt.editEnded    = ImGui::IsItemDeactivatedAfterEdit();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    EndPropertyRow();
    ImGui::PopID();

    return evt;
}
