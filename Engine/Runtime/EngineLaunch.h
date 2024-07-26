#pragma once

#include "EngineIncludes.h"

#include "Graphics/DXRenderManager.h"
#include "Graphics/Renderer/SpriteRenderer.h"
#include "SDL3/SDL.h"

DELTA_ENGINE_NS_BEGIN

enum class GameState {
	PLAY,
	EXIT,
	Error,
};

class EngineLaunch
{
public:
	EngineLaunch();

	void Initialize();
	void StartMainLoop();

	int exitCode;

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

	DXRenderManager* dxRenderManager;

	float time;
};

DELTA_ENGINE_NS_END
