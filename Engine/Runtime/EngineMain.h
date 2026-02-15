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
#include "SDL3/SDL.h"
#include "Core/Time.h"

DELTA_ENGINE_NS_BEGIN

enum class GameState {
	PLAY,
	EXIT,
	Error,
};

class EngineMain
{
public:
	EngineMain();

	/// Initialize with a scene renderer and window. Editor creates Device, SwapChain, offscreen RT
	/// and passes the scene renderer (DXRenderManager) that renders to that RT.
	void Initialize(std::shared_ptr<DXRenderManager> sceneRenderer, SDL_Window* window);

	/// Called each frame by the host (Editor/Game). Updates time.
	void Tick();

	/// Process an SDL event. Called by Editor for camera movement etc.
	void ProcessEvent(const SDL_Event& event);

	/// Called when window is resized. Updates camera aspect ratio.
	void OnWindowResized(UINT width, UINT height);

	/// Record scene draw calls to the command list. Called by Editor during RenderFrame.
	void RecordSceneDraws(std::shared_ptr<DXGraphicsContext> context);

	int exitCode;
	GameState gameState = GameState::PLAY;

	std::shared_ptr<DXRenderManager> dxRenderManager;

	inline std::shared_ptr<DXRenderManager> GetRenderManager() const { return dxRenderManager; }

	/// Cleanup before exit. Call when shutting down.
	void Cleanup();
    inline std::shared_ptr<DWorld> GetWorld() const { return m_world; }
	inline SDL_Window* GetWindow() const { return window; }

private:
	SDL_Renderer* renderer;
	SDL_Window* window;

    Time* time;

    std::shared_ptr<DWorld> m_world;
    std::shared_ptr<GameObject> m_cameraGameObject;
};

DELTA_ENGINE_NS_END
