// DeltaEngine.cpp : Defines the entry point for the application.
//

#include "DeltaEngine.h"
#include <fmt/core.h>
#include "SDL3/SDL.h"

#define SCREEN_WIDTH   1280
#define SCREEN_HEIGHT  720

typedef struct {
	SDL_Renderer* renderer;
	SDL_Window* window;
} App;

using namespace std;

App app;

void initSDL(void) {
    int rendererFlags, windowFlags;

    rendererFlags = SDL_RENDERER_ACCELERATED;

    windowFlags = SDL_WINDOW_OPENGL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    app.window = SDL_CreateWindow("Shooter 01", SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags);

    if (!app.window) {
        printf("Failed to open %d x %d window: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_GetError());
        exit(1);
    }

    //SDL_SetHint(SDLHint, "linear");

    app.renderer = SDL_CreateRenderer(app.window, "Test", SDL_RENDERER_ACCELERATED);

    if (!app.renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        exit(1);
    }
}

void doInput(void) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
            exit(0);
            break;

        default:
            break;
        }
    }
}

int main()
{
    initSDL();
    while (1) {

    }
	cout << "Hello CMake." << endl;
	fmt::print("Hello World!\n");
	return 0;
}
