#include "UIComponents/PropertyWidgets/PropertyWidgetUtil.h"

#include "UIComponents/UIComponentsEditorTheme.h"

#include "Style/EditorTheme.h"

#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

float BeginPropertyRow(const char* label, const EditorTheme::ThemeColors& c)
{
    if (!label)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "BeginPropertyRow: label was null (expected non-null C string for property label)");
        return ImGui::GetContentRegionAvail().x;
    }

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

void EndPropertyRow() {}

DELTA_ENGINE_NS_END
