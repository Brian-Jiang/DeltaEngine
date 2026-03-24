#pragma once

#include "EngineIncludes.h"
#include "Style/EditorTheme.h"
#include "UIComponents/WidgetEditEvent.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

inline WidgetEditEvent WidgetEditFromLastItem(bool valueChanged)
{
    WidgetEditEvent evt;
    evt.valueChanged = valueChanged;
    evt.editBegan    = ImGui::IsItemActivated();
    evt.editEnded    = ImGui::IsItemDeactivatedAfterEdit();
    return evt;
}

// Draws the label column and returns remaining width for the value widget.
inline float BeginPropertyRow(const char* label, const EditorTheme::ThemeColors& c)
{
    const float fs      = ImGui::GetFontSize();
    const float labelW  = fs * 6.5f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;

    ImVec2 screenPos = ImGui::GetCursorScreenPos();

    ImGui::PushClipRect(
        screenPos,
        ImVec2(screenPos.x + labelW, screenPos.y + ImGui::GetFrameHeight()),
        true);
    ImGui::GetWindowDrawList()->AddText(
        screenPos,
        ImGui::ColorConvertFloat4ToU32(c.TLabel),
        label);
    ImGui::PopClipRect();

    ImGui::Dummy(ImVec2(labelW, ImGui::GetTextLineHeight()));
    ImGui::SameLine(0.f, spacing);

    return ImGui::GetContentRegionAvail().x;
}

// Optional row footer hook; currently does nothing.
inline void EndPropertyRow() {}

DELTA_ENGINE_NS_END
