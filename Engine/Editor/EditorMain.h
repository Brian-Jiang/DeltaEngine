#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "EditorRenderManager.h"
#include "Runtime/EngineMain.h"

DELTA_ENGINE_NS_BEGIN

/// Editor application. Owns the main loop, window, and render pipeline.
class EditorMain
{
public:
    EditorMain();
    ~EditorMain();

    int Run();

private:
    void ProcessEvents();
    void Shutdown();

    std::unique_ptr<EditorRenderManager> m_renderManager;
    std::unique_ptr<EngineMain> m_engine;
    std::shared_ptr<SDL_Window> m_window;
    bool m_running = true;
    int m_exitCode = 0;

    static constexpr int DEFAULT_WIDTH = 1280;
    static constexpr int DEFAULT_HEIGHT = 720;
};

DELTA_ENGINE_NS_END
