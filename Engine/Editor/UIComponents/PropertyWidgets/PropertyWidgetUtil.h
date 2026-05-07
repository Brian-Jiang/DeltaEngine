#pragma once

#include "EditorIncludes.h"

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

float BeginPropertyRow(const char* label, const EditorTheme::ThemeColors& c);

void EndPropertyRow();

DELTA_ENGINE_NS_END
