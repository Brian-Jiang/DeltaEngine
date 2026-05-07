#include "UIComponents/PropertyWidgets/ReferenceField.h"

#include "UIComponents/PropertyWidgets/PropertyWidgetUtil.h"
#include "UIComponents/UIComponentsEditorTheme.h"

#include "Style/EditorTheme.h"

#include "imgui.h"

using namespace DeltaEngine;

void ReferenceField::Draw(const char* label, const char* displayName, bool isNull)
{
    if (!label)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "ReferenceField::Draw: label was null (expected non-null C string)");
        return;
    }
    if (!displayName)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "ReferenceField::Draw: displayName was null (expected non-null C string or use empty string)");
        displayName = "";
    }

    EditorTheme* theme = ResolveUIComponentsEditorTheme();
    if (!theme)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "ReferenceField::Draw: no EditorTheme (expected g_editor or UIComponents test theme override)");
        return;
    }
    const auto& c  = theme->colors;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (!dl)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "ReferenceField::Draw: ImGui::GetWindowDrawList() returned null (expected active window)");
        return;
    }

    ImGui::PushID(label);

    float availW = BeginPropertyRow(label, c);
    float fh     = ImGui::GetFrameHeight();
    float fs     = ImGui::GetFontSize();

    ImVec2 pos = ImGui::GetCursorScreenPos();

    dl->AddRectFilled(
        pos, ImVec2(pos.x + availW, pos.y + fh),
        ImGui::ColorConvertFloat4ToU32(c.DInput), 3.f);
    dl->AddRect(
        pos, ImVec2(pos.x + availW, pos.y + fh),
        ImGui::ColorConvertFloat4ToU32(c.BLight), 3.f);

    float textY = pos.y + (fh - fs) * 0.5f;
    ImGui::SetCursorScreenPos(ImVec2(pos.x + 4.f, textY));
    ImGui::PushStyleColor(ImGuiCol_Text, isNull ? c.TDim : c.TPrimary);
    ImGui::TextUnformatted(displayName);
    ImGui::PopStyleColor();

    ImGui::SetCursorScreenPos(ImVec2(pos.x + availW, pos.y));
    ImGui::Dummy(ImVec2(0.f, fh));

    EndPropertyRow();
    ImGui::PopID();
}
