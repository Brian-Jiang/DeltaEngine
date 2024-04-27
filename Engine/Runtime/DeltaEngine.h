// DeltaEngine.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "SDL3/SDL.h"

enum class GameState {
	PLAY,
	EXIT
};

class DeltaEngine
{
public:
	DeltaEngine();

	void Initialize();
	void StartMainLoop();

private:
	void InitSDL();
	void HandleInput();

	SDL_Renderer* renderer;
	SDL_Window* window;

	GameState gameState;
};
