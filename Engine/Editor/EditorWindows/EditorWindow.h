#pragma once

#include "EngineIncludes.h"

#include <concepts>

DELTA_ENGINE_NS_BEGIN

class EditorWindow
{
public:
    EditorWindow() = default;
    virtual ~EditorWindow() = default;

    /// Renders the editor window. Called by EditorMain during RenderFrame.
    virtual void Render() = 0;
    const char* m_title = "Editor Window";
    bool* m_open = nullptr;
};

template<typename T>
concept IsEditorWindow = std::derived_from<T, EditorWindow>;

DELTA_ENGINE_NS_END
