#include "UIComponents/PropertyWidgets/EnumField.h"

#include "UIComponents/PropertyWidgets/PropertyWidgetUtil.h"
#include "UIComponents/UIComponentsEditorTheme.h"

#include "Runtime/Reflection/DEnum.h"
#include "Style/EditorTheme.h"

#include "imgui.h"

#include <string>

using namespace DeltaEngine;

WidgetEditEvent EnumField::Draw(const char* label, int64_t* underlyingValue, DEnum* schema)
{
    if (!label || !underlyingValue)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "EnumField::Draw: expected non-null label and underlyingValue (label={}, underlyingValue={})",
            static_cast<const void*>(label), static_cast<const void*>(underlyingValue));
        return {};
    }

    EditorTheme* theme = ResolveUIComponentsEditorTheme();
    if (!theme)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "EnumField::Draw: no EditorTheme (expected g_editor or UIComponents test theme override)");
        return {};
    }
    const auto& c = theme->colors;

    ImGui::PushID(label);

    float availW = BeginPropertyRow(label, c);
    ImGui::SetNextItemWidth(availW);

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

    std::string preview;
    if (schema)
    {
        if (const DEnumEntry* entry = schema->FindEntryByValue(*underlyingValue))
            preview = entry->name;
        else
            preview = std::to_string(*underlyingValue);
    }
    else
    {
        preview = std::to_string(*underlyingValue);
    }

    WidgetEditEvent evt;
    bool changed = false;

    if (ImGui::BeginCombo("##enum", preview.c_str()))
    {
        if (schema)
        {
            for (const DEnumEntry& entry : schema->GetEntries())
            {
                const bool isSelected = entry.value == *underlyingValue;
                if (ImGui::Selectable(entry.name.c_str(), isSelected))
                {
                    if (*underlyingValue != entry.value)
                    {
                        *underlyingValue = entry.value;
                        changed = true;
                    }
                }
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    evt.valueChanged = changed;
    evt.editBegan    = ImGui::IsItemActivated();
    evt.editEnded    = ImGui::IsItemDeactivatedAfterEdit();

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(8);

    EndPropertyRow();
    ImGui::PopID();

    return evt;
}
