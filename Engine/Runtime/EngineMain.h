#pragma once

#include "EngineIncludes.h"

#include <DirectXMath.h>
#include <memory>
#include <vector>

#include "Graphics/DXRenderManager.h"
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

	void Initialize();
	void StartMainLoop();

	int exitCode;

	std::shared_ptr<DXRenderManager> dxRenderManager;

	inline std::shared_ptr<DXRenderManager> GetRenderManager() const { return dxRenderManager; }
    inline std::shared_ptr<DWorld> GetWorld() const { return m_world; }

	inline SDL_Window* GetWindow() const { return window; }

	// todo use g_engine global variable
	//static EngineMain* instance;

private:
	void InitSDL();
	void HandleInput();
	void Draw();

	SDL_Renderer* renderer;
	SDL_Window* window;

	GameState gameState;

	//InstancedDrawer* m_instancedDrawer;


    Time* time;

    std::shared_ptr<DWorld> m_world;
    std::shared_ptr<GameObject> m_cameraGameObject;

};

DELTA_ENGINE_NS_END
