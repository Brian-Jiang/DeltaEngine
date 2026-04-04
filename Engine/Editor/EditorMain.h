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

/// Editor application. Owns the main loop, window, and render pipeline.
class EditorMain
{
public:
    DELTAEDITOR_API EditorMain();
    DELTAEDITOR_API ~EditorMain();

    /// Runs the editor main loop until shutdown.
    DELTAEDITOR_API int Run();

    template <typename T>
        requires IsEditorWindow<T>
    std::shared_ptr<T> OpenEditorWindow()
    {
        std::shared_ptr<T> window = std::make_shared<T>();
        m_editorWindows.push_back(window);
        return window;
    }

    template <typename T>
        requires IsEditorWindow<T>
    std::shared_ptr<T> GetEditorWindow()
    {
        for (const auto& window : m_editorWindows)
        {
            if (auto casted = std::dynamic_pointer_cast<T>(window))
            {
                return casted;
            }
        }

        return nullptr;
    }

    /// Draws all open editor windows for the current frame.
    DELTAEDITOR_API void RenderEditorWindows();

    /// Returns the runtime engine owned by the editor.
    DELTAEDITOR_API EngineMain* GetEngine() { return m_engine.get(); }
    /// Returns the scene texture shown by the viewport window.
    DELTAEDITOR_API ImTextureID GetSceneTextureId() const;

    /// Sets the scene render target size and updates the runtime camera aspect ratio.
    DELTAEDITOR_API void SetSceneRenderSize(UINT width, UINT height);
    /// Returns the current scene render target size.
    DELTAEDITOR_API void GetSceneRenderSize(UINT& width, UINT& height) const;

    /// Sets a preview camera override that replaces the scene camera for the next frame.
    DELTAEDITOR_API void SetPreviewCameraOverride(const CameraCB& cb);
    /// Clears the preview camera override so the scene camera is used again.
    DELTAEDITOR_API void ClearPreviewCameraOverride();
    /// Returns the active editor theme.
    DELTAEDITOR_API EditorTheme* GetEditorTheme() { return m_editorTheme.get(); }

private:
    void ProcessEvents();
    void Shutdown();

    std::unique_ptr<EditorCore> m_editorCore;
    std::unique_ptr<EditorRenderManager> m_renderManager;
    std::unique_ptr<EngineMain> m_engine;
    std::shared_ptr<SDL_Window> m_window;
    std::vector<std::shared_ptr<EditorWindow>> m_editorWindows;
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
