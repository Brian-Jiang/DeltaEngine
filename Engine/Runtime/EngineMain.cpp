#include "EngineMain.h"

#include <chrono>
#include <vector>

#include "Graphics/DXUtils.h"
#include "Importers/ModelImporter.h"
#include "Graphics/DirectX/VertexAttributes.h"
#include "Graphics/Structures/Vertex.h"

#define SCREEN_WIDTH   1280
#define SCREEN_HEIGHT  720

using namespace DeltaEngine;
using namespace DirectX;

EngineMain* EngineMain::instance = nullptr;

EngineMain::EngineMain() : exitCode(0), renderer(nullptr), window(nullptr), gameState(GameState::PLAY),
time(0.0f), dxRenderManager(nullptr)
{
    EngineMain::instance = this;

    dxSprite = new SpriteRenderer();
    meshRenderer = new MeshRenderer();
	meshRenderer2 = new MeshRenderer();
}

void EngineMain::Initialize()
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    InitSDL();

    // std::ofstream file("relative_path_test.txt");
    //
    // if (file.is_open()) {
    //     file << "Test file";
    // }
    // file.close();

    //std::vector<Vertex> g_Vertices = {
    //    { XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f) }, // 0
    //    { XMFLOAT4(-1.0f,  1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) }, // 1
    //    { XMFLOAT4( 1.0f,  1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f) }, // 2
    //    { XMFLOAT4( 1.0f, -1.0f, -1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f) }, // 3
    //    { XMFLOAT4(-1.0f, -1.0f,  1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }, // 4
    //    { XMFLOAT4(-1.0f,  1.0f,  1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f) }, // 5
    //    { XMFLOAT4( 1.0f,  1.0f,  1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }, // 6
    //    { XMFLOAT4( 1.0f, -1.0f,  1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f) }  // 7
    //};

    //std::vector<unsigned int> g_Indicies =
    //{
    //    0, 1, 2, 0, 2, 3,
    //    4, 6, 5, 4, 7, 6,
    //    4, 5, 1, 4, 1, 0,
    //    3, 2, 6, 3, 6, 7,
    //    1, 5, 6, 1, 6, 2,
    //    4, 0, 3, 4, 3, 7
    //};

	std::vector<Texture*> textures;

    //auto mesh = new Mesh(g_Vertices, g_Indicies, textures);
	//meshRenderer2->Start(*mesh, dxRenderManager->GetDevice(), dxRenderManager->GetCommandList(), dxRenderManager->GetSRVHeap());

    auto importer = new ModelImporter();
    importer->Import("Assets/home/source/home.fbx");
    //importer->Import("Assets/car/source/datsun240k.fbx");
    meshRenderer->Start(importer->meshes, importer->meshTransforms, dxRenderManager->GetDevice(), dxRenderManager->GetCommandList(), dxRenderManager->GetSRVHeap());

    dxSprite->Start(-1.0f, -1.0f, 2.0f, 2.0f, "Assets/logo.png", dxRenderManager->GetDevice(), dxRenderManager->GetCommandList(), dxRenderManager->GetSRVHeap());

    dxRenderManager->InitFinish();
}

void ThrowIfFailed(HRESULT hresult)
{
	if (FAILED(hresult))
	{
		throw std::exception();
	}
}

void EngineMain::InitSDL() {
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

void EngineMain::StartMainLoop()
{
    eyePosition = XMVectorSet(0, 0, -10, 1);

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

void EngineMain::HandleInput()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        auto key = event.key.keysym.sym;
        switch (event.type) {
			case SDL_EVENT_WINDOW_RESIZED:
				dxRenderManager->Resize(event.window.data1, event.window.data2);
				break;
	        case SDL_EVENT_KEY_DOWN:
				if (key == SDLK_F11) {
					dxRenderManager->SetFullscreen(!dxRenderManager->IsFullscreen());
				} else if (key == SDLK_v)
				{
					dxRenderManager->ToggleVSync(!dxRenderManager->IsVSync());
				}
                else if (key == SDLK_DOWN || key == SDLK_s) {
					eyePosition -= XMVectorSet(0, 0, 1.f, 0);
                }
				else if (key == SDLK_UP || key == SDLK_w) {
					eyePosition += XMVectorSet(0, 0, 1.f, 0);
				}
				else if (key == SDLK_LEFT || key == SDLK_a) {
					eyePosition -= XMVectorSet(1.f, 0, 0, 0);
				}
				else if (key == SDLK_RIGHT || key == SDLK_d) {
					eyePosition += XMVectorSet(1.f, 0, 0, 0);
                }
                else if (key == SDLK_q) {
					eyePosition += XMVectorSet(0, 1.f, 0, 0);
				}
				else if (key == SDLK_e) {
					eyePosition -= XMVectorSet(0, 1.f, 0, 0);
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

void EngineMain::Draw()
{
    m_FoV = 45.0f;

    // Update the model matrix.
    float angle = static_cast<float>(time * 50.f);
    //angle = 0.f;
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 0, 0);
	auto translation = XMMatrixTranslation(0, 0, 900);
    auto rotation = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));
	m_ModelMatrix = XMMatrixMultiply(rotation, translation);
    //m_ModelMatrix = XMMatrixIdentity();

    // Update the view matrix.
    
	const XMVECTOR focusPoint = eyePosition + XMVectorSet(0, 0, 1, 0);
    const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    // Update the projection matrix.
    float aspectRatio = dxRenderManager->GetWidth() / static_cast<float>(dxRenderManager->GetHeight());
    m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FoV), aspectRatio, 0.1f, 100000.0f);

    mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_ViewMatrix);
    mvpMatrix = XMMatrixMultiply(mvpMatrix, m_ProjectionMatrix);
	dxRenderManager->SetMVPMatrix(mvpMatrix);
    //commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

    dxRenderManager->PrepareFrame();
    //dxSprite->Render(dxRenderManager->GetCommandList());
	meshRenderer->Render(dxRenderManager->GetCommandList(), dxRenderManager->GetSRVHeap(), dxRenderManager->GetDevice());
	//meshRenderer2->Render(dxRenderManager->GetCommandList(), dxRenderManager->GetSRVHeap(), dxRenderManager->GetDevice());
    dxRenderManager->RenderFrame();
}
