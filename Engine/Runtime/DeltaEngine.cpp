// DeltaEngine.cpp : Defines the entry point for the application.
//

#include "DeltaEngine.h"

#include <cstdio>
#include <fstream>
#include <iostream>

#include "../../ThirdParty/glew/include/GL/glew.h"
#include <SDL_opengl.h>

#include "SDL3/SDL.h"
#include "Const.h"


#define SCREEN_WIDTH   1280
#define SCREEN_HEIGHT  720


DeltaEngine::DeltaEngine() : exitCode(0), renderer(nullptr), window(nullptr), gameState(GameState::PLAY),
                             shader(nullptr), time(0.0f)
{
	sprite = new SpriteRenderer();
}

void DeltaEngine::Initialize()
{
    InitSDL();

    // std::ofstream file("relative_path_test.txt");
    //
    // if (file.is_open()) {
    //     file << "Test file";
    // }

    // file.close();

	
	sprite->Start(-0.5f, -0.5f, 1.0f, 1.0f, "assets/texture.png");
	shader = new GLSLProgram();
	shader->compileShaders("../../../../Engine/Runtime/Shaders/Vertex.glsl", "../../../../Engine/Runtime/Shaders/Fragment.glsl");
// shader->linkShaders();
// shader->addAttribute("vertexPosition");
// shader->addAttribute("vertexColor");
// shader->addAttribute("vertexUV");

// shader->unuse();
}

void DeltaEngine::InitSDL() {
    // int n = SDL_GetNumRenderDrivers();
    // for (size_t i = 0; i < n; i++) {
    //     //SDL_RendererInfo info;
    //     auto info = SDL_GetRenderDriver(i);
    //     printf(info);
    //     printf("\n");
    // }

    // SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "direct3d12", SDL_HINT_DEFAULT);

    int rendererFlags, windowFlags;

    rendererFlags = SDL_RENDERER_ACCELERATED;

    windowFlags = SDL_WINDOW_OPENGL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        gameState = GameState::Error;
    }

    window = SDL_CreateWindow("Shooter 01", SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags);

    if (!window) {
        printf("Failed to open %d x %d window: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_GetError());
        gameState = GameState::Error;
    }

    auto context = SDL_GL_CreateContext(window);
	if (!context) {
		printf("Failed to create OpenGL context: %s\n", SDL_GetError());
		gameState = GameState::Error;
	}

    auto error = glewInit();
    if (error != GLEW_OK) {
        const auto glewError = reinterpret_cast<const char*>(glewGetErrorString(error));
        printf("Error initializing GLEW: %s\n", glewError);
        gameState = GameState::Error;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    glClearColor(0.3f, 0.5f, 1.0f, 1.0f);

    //SDL_SetHint(SDLHint, "linear");

    // renderer = SDL_CreateRenderer(window, RENDERER_OPENGL, SDL_RENDERER_ACCELERATED);
    // if (!renderer) {
    //     printf("Failed to create renderer: %s\n", SDL_GetError());
    //     gameState = GameState::Error;
    // }
}

void DeltaEngine::StartMainLoop()
{
    while (gameState == GameState::PLAY) {
        // SDL_SetRenderDrawColor(renderer, 96, 128, 255, 255);
        // SDL_RenderClear(renderer);
		time += 0.01f;
        
        HandleInput();
        Draw();

        // int errorCode = SDL_RenderPresent(renderer);
    }

    if (gameState == GameState::Error)
    {
        printf("Error");
        exitCode = 1;
        SDL_Quit();
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
	        case SDL_EVENT_MOUSE_MOTION:
				// std::cout << "Mouse moved to x: " << event.motion.x << " y: " << event.motion.y << '\n';
				break;
            default:
                break;
        }
    }
}

void DeltaEngine::Draw()
{
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // glEnableClientState(GL_COLOR_ARRAY);
    // glBegin(GL_TRIANGLES);
    // glColor3f(1, 1, 1);
    // glVertex2f(0, 0);
    // glVertex2f(0.1f, -0.15f);
    // glVertex2f(0.1f, 0.15f);
    // glEnd();


    shader->use();

    auto timeLocation = shader->getUniformLocation("time");
    glUniform1f(timeLocation, time);
    sprite->Render();

    shader->unuse();

    SDL_GL_SwapWindow(window);
}
