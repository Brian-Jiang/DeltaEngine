// DeltaEngine.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "GLSLProgram.h"
#include "Graphics/Renderer/SpriteRenderer.h"
#include "Graphics/DXRenderManager.h"
#include "Graphics/Renderer/DXSpriteRenderer.h"
#include "SDL3/SDL.h"

enum class GameState {
	PLAY,
	EXIT,
	Error,
};

class DeltaEngine
{
public:
	DeltaEngine();

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

	SpriteRenderer* sprite;
	DXSpriteRenderer *dxSprite;
	GLSLProgram* shader;

	DXRenderManager* dxRenderManager;

	float time;
};
