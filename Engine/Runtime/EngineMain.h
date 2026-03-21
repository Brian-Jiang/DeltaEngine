#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <vector>
#include <Windows.h>

#include "Runtime/Core/UUID.h"
#include "Runtime/Core/WorldContext.h"

union SDL_Event;

DELTA_ENGINE_NS_BEGIN

struct DXGraphicsContext;

class Camera;
class DXRenderManager;
class DWorld;
class GameObject;
class Time;

enum class GameState
{
    PLAY,
    EXIT,
    Error,
};

class EngineMain
{
public:
    DELTAENGINE_API EngineMain();
    DELTAENGINE_API ~EngineMain();

    /// Stores the renderer used for scene rendering.
    DELTAENGINE_API void Initialize(std::shared_ptr<DXRenderManager> sceneRenderer);

    /// Updates world state before the frame tick.
    DELTAENGINE_API void PreTick();

    /// Advances the engine clock for the current frame.
    DELTAENGINE_API void Tick();

    /// Handles an SDL event routed from the host application.
    DELTAENGINE_API void ProcessEvent(const SDL_Event& event);

    /// Updates the active camera aspect ratio after a resize.
    DELTAENGINE_API void OnWindowResized(UINT width, UINT height);

    /// Records scene draw calls into the provided graphics context.
    DELTAENGINE_API void RecordSceneDraws(std::shared_ptr<DXGraphicsContext> context);

    /// Creates the editor world used by the runtime.
    DELTAENGINE_API void CreateWorld();

    /// Creates the default runtime scene objects.
    DELTAENGINE_API void CreateGameObjects();

    /// Loads a scene asset into the active editor world.
    DELTAENGINE_API void LoadScene(const AssetId& sceneAssetId);

    /// Exit code reported by the runtime host.
    int exitCode = 0;

    /// Current runtime state requested by the host loop.
    GameState gameState = GameState::PLAY;

    /// Returns the scene renderer assigned during initialization.
    std::shared_ptr<DXRenderManager> GetRenderManager() const { return dxRenderManager; }

    /// Returns the active runtime camera, if one exists.
    DELTAENGINE_API Camera* GetCamera();

    /// Releases world and renderer state before shutdown.
    DELTAENGINE_API void Cleanup();

    /// Returns the editor world, or nullptr if it was not created.
    DELTAENGINE_API DWorld* GetWorld() const;

private:
    std::shared_ptr<DXRenderManager> dxRenderManager;
    std::unique_ptr<Time> time;
    std::vector<WorldContext> m_worldContextList;
    GameObject* m_cameraGameObject = nullptr;
};

DELTA_ENGINE_NS_END
