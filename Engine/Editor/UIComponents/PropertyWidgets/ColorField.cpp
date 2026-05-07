#include "UIComponents/PropertyWidgets/ColorField.h"

#include "UIComponents/PropertyWidgets/PropertyWidgetUtil.h"
#include "UIComponents/UIComponentsEditorTheme.h"

#include "Style/EditorTheme.h"

#include "imgui.h"

using namespace DeltaEngine;

WidgetEditEvent ColorField::Draw(const char* label, float* values, bool hasAlpha)
{
    if (!label || !values)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "ColorField::Draw: expected non-null label and values (label={}, values={})",
            static_cast<const void*>(label), static_cast<const void*>(values));
        return {};
    }

    EditorTheme* theme = ResolveUIComponentsEditorTheme();
    if (!theme)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "ColorField::Draw: no EditorTheme (expected g_editor or UIComponents test theme override)");
        return {};
    }
    const auto& c   = theme->colors;
    ImFont*     mono = theme->GetMonoFont();

    ImGui::PushID(label);

    float availW = BeginPropertyRow(label, c);
    float fh     = ImGui::GetFrameHeight();
    WidgetEditEvent evt;

    ImVec4 swatchCol = {values[0], values[1], values[2], hasAlpha ? values[3] : 1.f};
    if (ImGui::ColorButton("##sw", swatchCol,
            ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
            ImVec2(fh * 0.9f, fh * 0.9f)))
    {
        ImGui::OpenPopup("##cpick");
    }
    evt.Merge(WidgetEditFromLastItem(false));

    ImGui::SameLine(0.f, 4.f);
    ImGui::SetNextItemWidth(-1.f);

    ImGuiColorEditFlags editFlags =
        ImGuiColorEditFlags_NoLabel |
        ImGuiColorEditFlags_Float |
        ImGuiColorEditFlags_NoPicker |
        ImGuiColorEditFlags_NoOptions;
    if (!hasAlpha)
        editFlags |= ImGuiColorEditFlags_NoAlpha;

    if (mono)
        ImGui::PushFont(mono);
    const bool ceChanged = ImGui::ColorEdit4("##ce", values, editFlags);
    if (mono)
        ImGui::PopFont();
    evt.Merge(WidgetEditFromLastItem(ceChanged));

    if (ImGui::BeginPopup("##cpick"))
    {
        ImGuiColorEditFlags pickerFlags =
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_DisplayRGB |
            ImGuiColorEditFlags_DisplayHex;
        if (!hasAlpha)
            pickerFlags |= ImGuiColorEditFlags_NoAlpha;
        const bool pkChanged = ImGui::ColorPicker4("##pk", values, pickerFlags);
        evt.Merge(WidgetEditFromLastItem(pkChanged));
        ImGui::EndPopup();
    }

    EndPropertyRow();
    ImGui::PopID();

    return evt;
}
