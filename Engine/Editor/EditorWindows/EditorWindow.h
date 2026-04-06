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
    /// open is the EditorWindowInfo::m_open flag; pass &open to ImGui::Begin.
    virtual void Render(bool& open) = 0;

    /// When true, RenderEditorWindows removes this window from the list once
    /// open becomes false (e.g. the user clicks the docked-tab X button).
    /// Singleton windows return false so they can be re-shown from the menu.
    virtual bool ShouldDestroyOnClose() const { return false; }

    /// When true, OpenEditorWindow prevents a second instance from being created.
    virtual bool IsSingleton() const { return true; }

    /// ImGui window title string.
    const char* m_title = "Editor Window";
};

/// True if T is an editor dock window type (derives from EditorWindow).
template<typename T>
concept IsEditorWindow = std::derived_from<T, EditorWindow>;

DELTA_ENGINE_NS_END
