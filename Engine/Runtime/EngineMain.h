#pragma once

#include "EngineIncludes.h"

#include "Graphics/DXRenderManager.h"
#include "Graphics/Renderer/SpriteRenderer.h"
#include "Graphics/Renderer/MeshRenderer.h"
#include "SDL3/SDL.h"

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

	DXRenderManager* dxRenderManager;

	static EngineMain* instance;

private:
	void InitSDL();
	void HandleInput();
	void Draw();

	SDL_Renderer* renderer;
	SDL_Window* window;

	GameState gameState;

	// SpriteRenderer* sprite;
	SpriteRenderer *dxSprite;
	// GLSLProgram* shader;

	MeshRenderer *meshRenderer;
	

	float time;
};

DELTA_ENGINE_NS_END
