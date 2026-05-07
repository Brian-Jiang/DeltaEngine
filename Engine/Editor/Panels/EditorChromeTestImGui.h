#pragma once

#include "EditorIncludes.h"

#include "Panels/EditorChromeContext.h"

#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class StatusBar;

DELTAEDITOR_API void EditorChrome_CreateTestImGuiContext(ImVec2 displaySize);
DELTAEDITOR_API void EditorChrome_DestroyTestImGuiContext();

DELTAEDITOR_API bool EditorChrome_Test_DrawAppHeader(const EditorChromeContext& ctx);
DELTAEDITOR_API bool EditorChrome_Test_DrawMainToolbar(const EditorChromeContext& ctx);
DELTAEDITOR_API bool EditorChrome_Test_DrawStatusBar(const EditorChromeContext& ctx, const StatusBar* bar);

DELTA_ENGINE_NS_END
