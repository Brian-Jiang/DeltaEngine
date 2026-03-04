#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <vector>

#include "EditorRenderManager.h"
#include "EditorSelectionState.h"
#include "Runtime/EngineMain.h"
#include "EditorWindows/EditorWindow.h"

#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class EditorMain;
class EditorTheme;

/// Global editor instance. Set during EditorMain construction, cleared on destruction.
extern EditorMain* g_editor;

/// Editor application. Owns the main loop, window, and render pipeline.
class EditorMain
{
public:
    EditorMain();
    ~EditorMain();

    int Run();

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

    void RenderEditorWindows();

    EngineMain* GetEngine() { return m_engine.get(); }
    ImTextureID GetSceneTextureId() const { return m_renderManager->GetSceneTextureId(); }

    /// Set scene render target size (driven by viewport settings). Syncs to EngineMain camera aspect.
    void SetSceneRenderSize(UINT width, UINT height);
    void GetSceneRenderSize(UINT& width, UINT& height) const;
    EditorSelectionState* GetSelectionState() { return m_selectionState.get(); }

private:
    void ProcessEvents();
    void Shutdown();

    std::unique_ptr<EditorRenderManager> m_renderManager;
    std::unique_ptr<EngineMain> m_engine;
    std::unique_ptr<EditorSelectionState> m_selectionState;
    std::shared_ptr<SDL_Window> m_window;
    std::vector<std::shared_ptr<EditorWindow>> m_editorWindows;
    std::unique_ptr<EditorTheme> m_editorTheme;
    bool m_running = true;
    int m_exitCode = 0;

    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 720;
};

DELTA_ENGINE_NS_END
