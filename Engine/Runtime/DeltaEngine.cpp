// DeltaEngine.cpp : Defines the entry point for the application.
//

#include "DeltaEngine.h"

#include <cstdio>

#include "SDL3/SDL.h"

#define SCREEN_WIDTH   1280
#define SCREEN_HEIGHT  720


DeltaEngine::DeltaEngine() : renderer(nullptr), window(nullptr), gameState(GameState::PLAY)
{
	
}

void DeltaEngine::Initialize()
{
    InitSDL();
}

void DeltaEngine::InitSDL() {
    // int n = SDL_GetNumRenderDrivers();
    // for (size_t i = 0; i < n; i++) {
    //     //SDL_RendererInfo info;
    //     auto info = SDL_GetRenderDriver(i);
    //     printf(info);
    //     printf("\n");
    // }

    SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "direct3d12", SDL_HINT_DEFAULT);

    int rendererFlags, windowFlags;

    rendererFlags = SDL_RENDERER_ACCELERATED;

    windowFlags = SDL_WINDOW_OPENGL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow("Shooter 01", SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags);

    if (!window) {
        printf("Failed to open %d x %d window: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_GetError());
        exit(1);
    }

    //SDL_SetHint(SDLHint, "linear");
    //SDL_RENDERER_SOFTWARE
    renderer = SDL_CreateRenderer(window, "direct3d12", SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        exit(1);
    }
}

void DeltaEngine::StartMainLoop()
{
	while (gameState == GameState::PLAY) {
		HandleInput();
	}
}

void DeltaEngine::HandleInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
	        case SDL_EVENT_QUIT:
				gameState = GameState::EXIT;
	            break;
	        default:
	            break;
        }
    }
}
