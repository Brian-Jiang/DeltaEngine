#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "Graphics/DXRenderManager.h"
#include "Graphics/DXGraphicsContext.h"
#include "Graphics/Renderer/SpriteRenderer.h"
#include "Graphics/Renderer/MeshRenderer.h"
#include "Graphics/DirectX/InstancedDrawer.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/WorldContext.h"
#include "Runtime/Core/UUID.h"
#include "SDL3/SDL.h"
#include "Core/Time.h"
#include "Core/Camera.h"

DELTA_ENGINE_NS_BEGIN

enum class GameState {
	PLAY,
	EXIT,
	Error,
};

class EngineMain
{
public:
	DELTAENGINE_API EngineMain();

	/// Initialize with a scene renderer and window. Editor creates Device, SwapChain, offscreen RT
	/// and passes the scene renderer (DXRenderManager) that renders to that RT.
    DELTAENGINE_API void Initialize(std::shared_ptr<DXRenderManager> sceneRenderer, std::shared_ptr<SDL_Window>);

	DELTAENGINE_API void PreTick();

	/// Called each frame by the host (Editor/Game). Updates time.
	DELTAENGINE_API void Tick();

	/// Process an SDL event. Called by Editor for camera movement etc.
	DELTAENGINE_API void ProcessEvent(const SDL_Event& event);

	/// Called when window is resized. Updates camera aspect ratio.
	DELTAENGINE_API void OnWindowResized(UINT width, UINT height);

	/// Record scene draw calls to the command list. Called by Editor during RenderFrame.
	DELTAENGINE_API void RecordSceneDraws(std::shared_ptr<DXGraphicsContext> context);

	DELTAENGINE_API void CreateWorld();
    DELTAENGINE_API void CreateGameObjects();

	/// Load a DScene from the asset database by asset ID.
	/// All GameObjects in the scene are added to the Editor world.
	/// The first scene loaded automatically becomes the active scene.
	DELTAENGINE_API void LoadScene(const AssetId& sceneAssetId);

	int exitCode;
	GameState gameState = GameState::PLAY;

	std::shared_ptr<DXRenderManager> dxRenderManager;

	inline std::shared_ptr<DXRenderManager> GetRenderManager() const { return dxRenderManager; }

	/// Returns the main editor camera. May be null if not initialized.
	DELTAENGINE_API Camera* GetCamera();

	/// Cleanup before exit. Call when shutting down.
	DELTAENGINE_API void Cleanup();

	/// Returns the first Editor world, or nullptr if none exists.
	DELTAENGINE_API DWorld* GetWorld() const;

	inline std::shared_ptr<SDL_Window> GetWindow() const { return m_window; }

private:
	std::shared_ptr<SDL_Window> m_window;

    std::unique_ptr<Time> time;

    std::vector<WorldContext> m_worldContextList;
    GameObject* m_cameraGameObject;
};

DELTA_ENGINE_NS_END
