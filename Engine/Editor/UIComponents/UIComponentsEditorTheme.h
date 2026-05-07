#pragma once

#include "EditorIncludes.h"

DELTA_ENGINE_NS_BEGIN

class EditorTheme;

DECLARE_LOG_CATEGORY(LogUIComponents)

DELTAEDITOR_API EditorTheme* ResolveUIComponentsEditorTheme();

DELTAEDITOR_API void SetUIComponentsEditorThemeForTests(EditorTheme* theme);

DELTA_ENGINE_NS_END
