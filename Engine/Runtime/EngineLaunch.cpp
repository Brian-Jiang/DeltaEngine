#include "EngineLaunch.h"

#include <chrono>

#include "Graphics/DXUtils.h"
#include "Importers/ModelImporter.h"

#define SCREEN_WIDTH   1280
#define SCREEN_HEIGHT  720

using namespace DeltaEngine;

EngineLaunch::EngineLaunch() : exitCode(0), renderer(nullptr), window(nullptr), gameState(GameState::PLAY),
                             time(0.0f), dxRenderManager(nullptr), dxSprite(nullptr)
{
    dxSprite = new SpriteRenderer();
}

void EngineLaunch::Initialize()
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    InitSDL();

    // std::ofstream file("relative_path_test.txt");
    //
    // if (file.is_open()) {
    //     file << "Test file";
    // }
    // file.close();

    auto importer = new ModelImporter();
    importer->Import("");

    dxSprite->Start(-1.0f, -1.0f, 2.0f, 2.0f, "../../Engine/Runtime/Assets/Frame1.png", dxRenderManager->GetDevice(), dxRenderManager->GetCommandList(), dxRenderManager->GetSRVHeap());

    dxRenderManager->InitFinish();
}

void ThrowIfFailed(HRESULT hresult)
{
	if (FAILED(hresult))
	{
		throw std::exception();
	}
}

void EngineLaunch::InitSDL() {
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

    windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        gameState = GameState::Error;
    }

    window = SDL_CreateWindow("Delta Editor", SCREEN_WIDTH, SCREEN_HEIGHT, windowFlags);

    if (!window) {
        printf("Failed to open %d x %d window: %s\n", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_GetError());
        gameState = GameState::Error;
    }

    auto hwnd = static_cast<HWND>(SDL_GetProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    dxRenderManager = new DXRenderManager(hwnd, SCREEN_WIDTH, SCREEN_HEIGHT);

 //    auto context = SDL_GL_CreateContext(window);
	// if (!context) {
	// 	printf("Failed to create OpenGL context: %s\n", SDL_GetError());
	// 	gameState = GameState::Error;
	// }
 //
 //    auto error = glewInit();
 //    if (error != GLEW_OK) {
 //        const auto glewError = reinterpret_cast<const char*>(glewGetErrorString(error));
 //        printf("Error initializing GLEW: %s\n", glewError);
 //        gameState = GameState::Error;
 //    }

    // SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    // glClearColor(0.3f, 0.5f, 1.0f, 1.0f);



    //SDL_SetHint(SDLHint, "linear");

    // renderer = SDL_CreateRenderer(window, RENDERER_OPENGL, SDL_RENDERER_ACCELERATED);
    // if (!renderer) {
    //     printf("Failed to create renderer: %s\n", SDL_GetError());
    //     gameState = GameState::Error;
    // }
}

void EngineLaunch::StartMainLoop()
{
    while (gameState == GameState::PLAY) {
        // SDL_SetRenderDrawColor(renderer, 96, 128, 255, 255);
        // SDL_RenderClear(renderer);
        static std::chrono::high_resolution_clock clock;

		time += 0.01f;
        
        HandleInput();
        Draw();

        // int errorCode = SDL_RenderPresent(renderer);
    }

    if (gameState == GameState::Error)
    {
        printf("Error");
        exitCode = 1;
    }

    dxRenderManager->OnDestroy();

    // DXUtils::ReportLiveDXGIObjects();
    
    SDL_Quit();
}

void EngineLaunch::HandleInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
			case SDL_EVENT_WINDOW_RESIZED:
				dxRenderManager->Resize(event.window.data1, event.window.data2);
				break;
	        case SDL_EVENT_KEY_DOWN:
				if (event.key.keysym.sym == SDLK_F11) {
					dxRenderManager->SetFullscreen(!dxRenderManager->IsFullscreen());
				} else if (event.key.keysym.sym == SDLK_v)
				{
					dxRenderManager->ToggleVSync(!dxRenderManager->IsVSync());
				}
            break;
            case SDL_EVENT_QUIT:
                gameState = GameState::EXIT;
                break;
	   //      case SDL_EVENT_MOUSE_MOTION:
				// // std::cout << "Mouse moved to x: " << event.motion.x << " y: " << event.motion.y << '\n';
				// break;
            default:
                break;
        }
    }
}

void EngineLaunch::Draw()
{
    dxRenderManager->PrepareFrame();
    dxSprite->Render(dxRenderManager->GetCommandList());
    dxRenderManager->RenderFrame();
}
