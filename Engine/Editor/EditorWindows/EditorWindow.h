#pragma once

#include "EngineIncludes.h"

#include <concepts>
#include <string>

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

    /// Assigns the per-type instance id and rebuilds the ImGui window id string.
    /// Called by OpenEditorWindow after construction.
    void SetWindowId(int id)
    {
        m_id = id;
        m_imguiTitle = m_title + "##" + std::to_string(id);
    }

    /// Returns the ImGui window title ("Title##id").
    const char* GetImGuiTitle() const { return m_imguiTitle.c_str(); }

    /// Human-readable display title. Subclasses set this in their constructor.
    std::string m_title = "Editor Window";

    /// Per-type instance index (0-based), set by OpenEditorWindow.
    int m_id = 0;

private:
    std::string m_imguiTitle = "Editor Window##0";
};

/// True if T is an editor dock window type (derives from EditorWindow).
template<typename T>
concept IsEditorWindow = std::derived_from<T, EditorWindow>;

DELTA_ENGINE_NS_END
