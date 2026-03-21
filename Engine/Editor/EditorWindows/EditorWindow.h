#pragma once

#include "EngineIncludes.h"

#include <concepts>

DELTA_ENGINE_NS_BEGIN

class EditorWindow
{
public:
    EditorWindow() = default;
    virtual ~EditorWindow() = default;

    /// Called each frame from the editor render path to draw this window.
    virtual void Render() = 0;
    /// ImGui window title string.
    const char* m_title = "Editor Window";
    /// Optional open flag for ImGui::Begin; when null, no collapse close button is shown.
    bool* m_open = nullptr;
};

/// True if T is an editor dock window type (derives from EditorWindow).
template<typename T>
concept IsEditorWindow = std::derived_from<T, EditorWindow>;

DELTA_ENGINE_NS_END
