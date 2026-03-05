#include "UIComponents/PropertyWidgets/ReferenceField.h"
#include "UIComponents/PropertyWidgets/PropertyWidgetUtil.h"

#include "EditorMain.h"
#include "Style/EditorTheme.h"
#include "imgui.h"

using namespace DeltaEngine;

void ReferenceField::Draw(const char* label, const char* displayName, bool isNull)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;
    ImDrawList*  dl    = ImGui::GetWindowDrawList();

    ImGui::PushID(label);

    float availW = BeginPropertyRow(label, c);
    float fh     = ImGui::GetFrameHeight();
    float fs     = ImGui::GetFontSize();

    ImVec2 pos = ImGui::GetCursorScreenPos();

    // Background and border drawn via DrawList (read-only slot appearance)
    dl->AddRectFilled(
        pos, ImVec2(pos.x + availW, pos.y + fh),
        ImGui::ColorConvertFloat4ToU32(c.DInput), 3.f);
    dl->AddRect(
        pos, ImVec2(pos.x + availW, pos.y + fh),
        ImGui::ColorConvertFloat4ToU32(c.BLight), 3.f);

    // Text inside slot
    float textY = pos.y + (fh - fs) * 0.5f;
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 4.f, textY));
    ImGui::PushStyleColor(ImGuiCol_Text, isNull ? c.TDim : c.TPrimary);
    ImGui::TextUnformatted(displayName);
    ImGui::PopStyleColor();

    // Advance cursor past the slot so the next widget is positioned correctly
    ImGui::SetCursorScreenPos(ImVec2(pos.x + availW, pos.y));
    ImGui::Dummy(ImVec2(0.f, fh));

    EndPropertyRow();
    ImGui::PopID();
}
