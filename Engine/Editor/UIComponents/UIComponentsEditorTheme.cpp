#include "UIComponents/UIComponentsEditorTheme.h"

#include "Editor/EditorMain.h"
#include "Editor/Style/EditorTheme.h"

DELTA_ENGINE_NS_BEGIN

DEFINE_LOG_CATEGORY(LogUIComponents)

namespace
{
EditorTheme* g_uiComponentsThemeOverride = nullptr;
}

EditorTheme* ResolveUIComponentsEditorTheme()
{
    if (g_uiComponentsThemeOverride)
        return g_uiComponentsThemeOverride;
    return g_editor ? g_editor->GetEditorTheme() : nullptr;
}

void SetUIComponentsEditorThemeForTests(EditorTheme* theme)
{
    g_uiComponentsThemeOverride = theme;
}

DELTA_ENGINE_NS_END
