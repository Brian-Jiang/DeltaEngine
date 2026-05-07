#pragma once

#include "EditorIncludes.h"

DELTA_ENGINE_NS_BEGIN

class EditorTheme;
class EditorCore;
class EditorMain;

DECLARE_LOG_CATEGORY(LogEditorChrome)

struct EditorChromeContext
{
    EditorTheme* theme = nullptr;
    EditorCore* core = nullptr;
    EditorMain* editor = nullptr;
};

DELTA_ENGINE_NS_END
