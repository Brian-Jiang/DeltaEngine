#pragma once

#include "EditorIncludes.h"

#include <memory>
#include <vector>
#include <Windows.h>

#include "EditorWindows/EditorWindow.h"
#include "Runtime/Graphics/Structures/Camera.h"

#include "imgui.h"

struct SDL_Window;

DELTA_ENGINE_NS_BEGIN

class EditorCore;
class EditorMain;
class EditorRenderManager;
class EditorTheme;
class EngineMain;

/// Global editor instance. Set during EditorMain construction, cleared on destruction.
extern EditorMain* g_editor;

/// Tracks a single open editor window together with its visibility state.
struct EditorWindowInfo
{
    bool m_open = true;
    std::shared_ptr<EditorWindow> m_window;
};

/// Editor application. Owns the main loop, window, and render pipeline.
class EditorMain
{
public:
    DELTAEDITOR_API EditorMain();
    DELTAEDITOR_API ~EditorMain();

    /// Runs the editor main loop until shutdown.
    DELTAEDITOR_API int Run();

    /// Opens an editor window of type T.
    /// For singleton windows (IsSingleton() == true), re-shows the existing instance
    /// instead of creating a duplicate. Returns a raw pointer to the window.
    template <typename T>
        requires IsEditorWindow<T>
    T* OpenEditorWindow()
    {
        auto window = std::make_shared<T>();
        if (window->IsSingleton())
        {
            for (auto& info : m_editorWindows)
            {
                if (auto* existing = dynamic_cast<T*>(info.m_window.get()))
                {
                    info.m_open = true;
                    return existing;
                }
            }
        }
        int instanceCount = 0;
        for (auto& info : m_editorWindows)
        {
            if (dynamic_cast<T*>(info.m_window.get()))
                ++instanceCount;
        }
        window->SetWindowId(instanceCount);
        EditorWindowInfo& info = m_editorWindows.emplace_back();
        info.m_open = true;
        info.m_window = window;
        return window.get();
    }

    template <typename T>
        requires IsEditorWindow<T>
    T* GetEditorWindow()
    {
        for (auto& info : m_editorWindows)
        {
            if (auto* casted = dynamic_cast<T*>(info.m_window.get()))
                return casted;
        }
        return nullptr;
    }

    /// Draws all open editor windows for the current frame.
    DELTAEDITOR_API void RenderEditorWindows();

    /// Returns the runtime engine owned by the editor.
    DELTAEDITOR_API EngineMain* GetEngine() { return m_engine.get(); }
    /// Returns the editor render manager that owns the toolbar and swap chain.
    DELTAEDITOR_API EditorRenderManager* GetRenderManager() { return m_renderManager.get(); }
    /// Returns the scene texture shown by the viewport window.
    DELTAEDITOR_API ImTextureID GetSceneTextureId() const;

    /// Sets the scene render target size and updates the runtime camera aspect ratio.
    DELTAEDITOR_API void SetSceneRenderSize(UINT width, UINT height);
    /// Returns the current scene render target size.
    DELTAEDITOR_API void GetSceneRenderSize(UINT& width, UINT& height) const;

    /// Sets the preview / viewport camera for the next frame (shadow pass + scene draws).
    DELTAEDITOR_API void SetActiveRenderCamera(const ActiveRenderCamera& camera);
    DELTAEDITOR_API void ClearActiveRenderCamera();
    /// Returns the active editor theme.
    DELTAEDITOR_API EditorTheme* GetEditorTheme() { return m_editorTheme.get(); }
    /// Returns the list of all open editor windows and their visibility state.
    /// Used by the Window menu to re-show singleton windows that were closed.
    DELTAEDITOR_API std::vector<EditorWindowInfo>& GetEditorWindowInfos();

    /// Opens a new viewport window instance.
    DELTAEDITOR_API void OpenViewportWindow();

private:
    void ProcessEvents();
    void Shutdown();

    std::unique_ptr<EditorCore> m_editorCore;
    std::unique_ptr<EditorRenderManager> m_renderManager;
    std::unique_ptr<EngineMain> m_engine;
    std::shared_ptr<SDL_Window> m_window;
    std::vector<EditorWindowInfo> m_editorWindows;
    std::unique_ptr<EditorTheme> m_editorTheme;
    bool m_sdlInitialized = false;
    bool m_imguiContextCreated = false;
    bool m_imguiSdlInitialized = false;
    bool m_imguiDx12Initialized = false;
    bool m_running = true;
    int m_exitCode = 0;

    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 720;
};

DELTA_ENGINE_NS_END
