#include "EngineMain.h"

#include <vector>
#include <memory>

#include "Graphics/DXUtils.h"
#include "Importers/ModelImporter.h"
#include "Graphics/DirectX/VertexAttributes.h"
#include "Graphics/Structures/Vertex.h"

#define SCREEN_WIDTH   1280
#define SCREEN_HEIGHT  720

using namespace DeltaEngine;
using namespace DirectX;

EngineMain* EngineMain::instance = nullptr;

EngineMain::EngineMain() 
    : exitCode(0), renderer(nullptr), window(nullptr), 
    gameState(GameState::PLAY), dxRenderManager(nullptr), time(nullptr)
{
    EngineMain::instance = this;

    dxSprite = new SpriteRenderer();
    meshRenderer = new MeshRenderer();
    meshRenderer2 = new MeshRenderer();
    time = new Time();
}

void EngineMain::Initialize()
{
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    InitSDL();

    std::vector<Texture*> textures;

    //auto mesh = new Mesh(g_Vertices, g_Indicies, textures);
    //meshRenderer2->Start(*mesh, dxRenderManager->GetDevice(), dxRenderManager->GetCommandList(), dxRenderManager->GetSRVHeap());

    auto importer = new ModelImporter();
    importer->Import("Assets/cottage/source/dio.fbx");
    //importer->Import("Assets/weapon/weapon.fbx");
    //importer->Import("Assets/home/source/home.fbx");
    //importer->Import("Assets/car/source/datsun240k.fbx");
    meshRenderer->Start(importer->meshes, importer->meshTransforms, dxRenderManager->GetDevice(), dxRenderManager->GetCommandList(), dxRenderManager->GetSRVHeap());

    std::shared_ptr<Mesh> meshPtr(importer->meshes[0]);
    m_instancedDrawer = new InstancedDrawer(meshPtr, 100);
    for (size_t i = 0; i < 100; i++) {
        auto tranlate = XMMatrixTranslation(i / 10.0f, i / 10.0f, 0.0f);
        
        m_instancedDrawer->SetTransform(i, tranlate);
    }

    m_instancedDrawer->CreateBuffer(dxRenderManager->GetDevice());
    //auto instanceBuffer = m_instancedDrawer->GetInstanceBuffer();

    //dxSprite->Start(-1.0f, -1.0f, 2.0f, 2.0f, "Assets/logo.png", dxRenderManager->GetDevice(), dxRenderManager->GetCommandList(), dxRenderManager->GetSRVHeap());

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
}

void EngineMain::StartMainLoop()
{
    eyePosition = XMVectorSet(0, 0, -10, 1);

    while (gameState == GameState::PLAY) {
        time->TickTime();
        
        HandleInput();
        Draw();
    }

    if (gameState == GameState::Error)
    {
        printf("Error");
        exitCode = 1;
    }

    dxRenderManager->OnDestroy();
    
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
    float angle = static_cast<float>(Time::timeSinceStart * 50.f);
    //angle = 0.f;
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 0, 0);
    auto translation = XMMatrixTranslation(0, 0, 0);
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
    dxRenderManager->SetCameraPosition(eyePosition);
    //commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);

    dxRenderManager->PrepareFrame();
    //dxSprite->Render(dxRenderManager->GetCommandList());
    meshRenderer->Render(dxRenderManager->GetCommandList(), dxRenderManager->GetSRVHeap(), dxRenderManager->GetDevice());
    //meshRenderer2->Render(dxRenderManager->GetCommandList(), dxRenderManager->GetSRVHeap(), dxRenderManager->GetDevice());
    m_instancedDrawer->Draw(dxRenderManager->GetCommandList());
    dxRenderManager->RenderFrame();
}
