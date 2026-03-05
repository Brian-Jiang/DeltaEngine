#pragma once

#include "EngineIncludes.h"
#include "Style/EditorTheme.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

// Renders the label column (width = fs * 6.5f) via DrawList, advances cursor into the value region.
// Returns the available width for the value widget.
inline float BeginPropertyRow(const char* label, const EditorTheme::ThemeColors& c)
{
    const float fs      = ImGui::GetFontSize();
    const float labelW  = fs * 6.5f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;

    ImVec2 screenPos = ImGui::GetCursorScreenPos();

    // Clip label rendering to the label column width
    ImGui::PushClipRect(
        screenPos,
        ImVec2(screenPos.x + labelW, screenPos.y + ImGui::GetFrameHeight()),
        true);
    ImGui::GetWindowDrawList()->AddText(
        screenPos,
        ImGui::ColorConvertFloat4ToU32(c.TLabel),
        label);
    ImGui::PopClipRect();

    // Advance cursor by the label column (use Dummy so ImGui tracks item size)
    ImGui::Dummy(ImVec2(labelW, ImGui::GetTextLineHeight()));
    ImGui::SameLine(0.f, spacing);

    return ImGui::GetContentRegionAvail().x;
}

// No-op — reserved for future row hover highlight.
inline void EndPropertyRow() {}

DELTA_ENGINE_NS_END
