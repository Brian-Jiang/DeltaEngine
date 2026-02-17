#include "EngineMain.h"

#include <vector>
#include <memory>

#include "Graphics/DXUtils.h"
#include "Core/DMesh.h"
#include "Graphics/Structures/Vertex.h"
#include "Graphics/Light/DirectionalLight.h"
#include "Graphics/Light/PointLight.h"
#include "Graphics/Light/SpotLight.h"
#include "Graphics/DirectX/CommandList.h"
#include "Runtime/Core/GameObject.h"
#include "Core/DShader.h"
#include "Core/DMaterial.h"
#include "Core/Camera.h"

#define SCREEN_WIDTH   1280
#define SCREEN_HEIGHT  720

using namespace DeltaEngine;
using namespace DirectX;

EngineMain::EngineMain()
    : exitCode(0)
    , gameState(GameState::PLAY)
{
    time = std::make_unique<Time>();
}

void EngineMain::Initialize(std::shared_ptr<DXRenderManager> sceneRenderer, std::shared_ptr<SDL_Window> window)
{
    dxRenderManager = std::move(sceneRenderer);
    m_window = window;

    // ---- Build the scene world and add renderers ----
    m_world = std::make_shared<DWorld>();


    // ---------- game object: star mesh renderer
    {
        std::shared_ptr<GameObject> go = m_world->CreateGameObject("MeshRenderer");
        std::shared_ptr<MeshRenderer> meshRenderer = go->AddSceneComponent<MeshRenderer>();
        meshRenderer->SetLocalPosition(2.0f, 0.0f, 5.0f);
        std::shared_ptr<DShader> shader = std::make_shared<DShader>(
            L"Shaders.hlsl",
            L"VSMain", L"PSMain",
            L"vs_6_0", L"ps_6_0");

        shader->SetInputLayout({
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        });
        //std::shared_ptr<DMaterial> material = std::make_shared<DMaterial>(shader);
        std::shared_ptr<DMesh> mesh = std::make_shared<DMesh>(std::wstring(L"Star.obj"));
        std::vector<std::shared_ptr<DTexture>> textures = mesh->GetTextures();
        std::vector<std::shared_ptr<DMaterial>> materials;
        for (int i = 0; i < mesh->GetSubMeshCount(); i++)
        {
            std::shared_ptr<DMaterial> material = std::make_shared<DMaterial>(shader);
            if (i < textures.size())
            {
                material->AddTexture(textures[i]);
            }
            materials.push_back(material);
        }

        mesh->SetMaterials(materials);
        meshRenderer->SetMesh(mesh);
    }

    // ---------- game object: home mesh renderer
    {
        std::shared_ptr<GameObject> go = m_world->CreateGameObject("HomeMeshRenderer");
        std::shared_ptr<MeshRenderer> meshRenderer = go->AddSceneComponent<MeshRenderer>();
        meshRenderer->SetLocalPosition(0.0f, 0.0f, 0.0f);
        std::shared_ptr<DShader> shader = std::make_shared<DShader>(
            L"Shaders.hlsl",
            L"VSMain", L"PSMain",
            L"vs_6_0", L"ps_6_0");

        shader->SetInputLayout({
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        });
        //std::shared_ptr<DMaterial> material = std::make_shared<DMaterial>(shader);
        std::shared_ptr<DMesh> mesh = std::make_shared<DMesh>(std::wstring(L"home/source/home.fbx"));
        std::vector<std::shared_ptr<DTexture>> textures = mesh->GetTextures();
        std::vector<std::shared_ptr<DMaterial>> materials;
        for (int i = 0; i < mesh->GetSubMeshCount(); i++)
        {
            std::shared_ptr<DMaterial> material = std::make_shared<DMaterial>(shader);
            if (i < textures.size())
            {
                material->AddTexture(textures[i]);
            }
            materials.push_back(material);
        }

        mesh->SetMaterials(materials);
        meshRenderer->SetMesh(mesh);
    }


    // ---------- game object: camera
    std::shared_ptr<GameObject> cameraGo = m_world->CreateGameObject("Camera");
    m_cameraGameObject = cameraGo;
    std::shared_ptr<Camera> camera = cameraGo->AddSceneComponent<Camera>();
    camera->SetLocalPosition(0.0f, 50.0f, -400.0f);
    camera->UpdateParameters(DirectX::XM_PIDIV4, static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT, 0.1f, 1000.0f);


    // ---------- game object: directional light (sun)
    std::shared_ptr<GameObject> directionalLightGo = m_world->CreateGameObject("DirectionalLight");
    std::shared_ptr<DirectionalLight> directionalLight = directionalLightGo->AddSceneComponent<DirectionalLight>();
    directionalLight->SetLocalRotation(DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitX, XM_PIDIV4));
    directionalLight->UpdateParameters(XMVectorSet(0, -1, 0, 0), XMVectorSet(1.0f, 1.0f, 0.95f, 1.0f), 0.7f);

    // ---------- game object: point light
    std::shared_ptr<GameObject> pointLightGo = m_world->CreateGameObject("PointLight");
    std::shared_ptr<PointLight> pointLight = pointLightGo->AddSceneComponent<PointLight>();
    pointLight->SetLocalPosition(0.0f, 3.0f, 2.0f);
    pointLight->UpdateParameters(XMVectorSet(1.0f, 0.4f, 0.2f, 1.0f), 2.0f, 15.0f);

    // ---------- game object: spot light
    std::shared_ptr<GameObject> spotLightGo = m_world->CreateGameObject("SpotLight");
    std::shared_ptr<SpotLight> spotLight = spotLightGo->AddSceneComponent<SpotLight>();
    spotLight->SetLocalPosition(-2.0f, 4.0f, 0.0f);
    spotLight->SetLocalRotation(DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitZ, -XM_PIDIV4));
    spotLight->UpdateParameters(XMVectorSet(0.2f, 0.8f, 1.0f, 1.0f), 3.0f, 20.0f, XM_PI / 6.0f, XM_PI / 3.0f);
    

    // D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
    //     { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //     { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //     { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //     { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

    //    // Instance data (per-instance, unique to each instance)
    //    { "INSTANCE_WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    //    { "INSTANCE_WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    //    { "INSTANCE_WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    //    { "INSTANCE_WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    //    { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    //};

    // ---- Import model and set up instanced drawing ----
    // auto importer = new ModelImporter();
    // importer->Import("Assets/Star.obj");

    // std::shared_ptr<Mesh> meshPtr(importer->meshes[0]);
    // m_instancedDrawer = new InstancedDrawer(meshPtr, 10000);
    // UINT32 idx = 0;
    // for (size_t i = 0; i < 100; i++) {
    //     for (size_t j = 0; j < 100; j++) {
    //         auto tranlate = XMMatrixTranslation(i / 10.0f, j / 10.0f, 0.0f);
    //         //m_instancedDrawer->SetTransform(idx, tranlate);
    //         XMFLOAT3 color(1.0f, 0.843f, 0.0f);
    //         //m_instancedDrawer->SetColor(idx, color);
    //         ++idx;
    //     }
    // }

    // m_instancedDrawer->CreateBuffer(dxRenderManager->GetDevice());


    //auto spriteRenderer = go->AddSceneComponent<SpriteRenderer>();
    //spriteRenderer->Start(2.0f, 2.0f, "Assets/logo.png");


    dxRenderManager->InitWorldRenderers(*m_world);

    //atexit(&Device::ReportLiveObjects);
}

void DeltaEngine::EngineMain::PreTick()
{
    m_world->PreTick(Time::deltaTime);
}

void EngineMain::Tick()
{
    time->TickTime();
}

void EngineMain::ProcessEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_KEY_DOWN && m_cameraGameObject)
    {
        SDL_Keycode key = event.key.key;
        if (key == SDLK_DOWN || key == SDLK_S)
            m_cameraGameObject->GetRootSceneComponent()->SetLocalPosition(m_cameraGameObject->GetRootSceneComponent()->GetLocalPosition() - XMFLOAT3(0, 0, 1.f));
        else if (key == SDLK_UP || key == SDLK_W)
            m_cameraGameObject->GetRootSceneComponent()->SetLocalPosition(m_cameraGameObject->GetRootSceneComponent()->GetLocalPosition() + XMFLOAT3(0, 0, 1.f));
        else if (key == SDLK_LEFT || key == SDLK_A)
            m_cameraGameObject->GetRootSceneComponent()->SetLocalPosition(m_cameraGameObject->GetRootSceneComponent()->GetLocalPosition() - XMFLOAT3(1.f, 0, 0));
        else if (key == SDLK_RIGHT || key == SDLK_D)
            m_cameraGameObject->GetRootSceneComponent()->SetLocalPosition(m_cameraGameObject->GetRootSceneComponent()->GetLocalPosition() + XMFLOAT3(1.f, 0, 0));
        else if (key == SDLK_Q)
            m_cameraGameObject->GetRootSceneComponent()->SetLocalPosition(m_cameraGameObject->GetRootSceneComponent()->GetLocalPosition() - XMFLOAT3(0, 1.f, 0));
        else if (key == SDLK_E)
            m_cameraGameObject->GetRootSceneComponent()->SetLocalPosition(m_cameraGameObject->GetRootSceneComponent()->GetLocalPosition() + XMFLOAT3(0, 1.f, 0));
    }
}

void EngineMain::OnWindowResized(UINT width, UINT height)
{
    if (m_cameraGameObject)
        m_cameraGameObject->GetRootSceneComponent<Camera>()->UpdateAspectRatio(static_cast<float>(width) / height);
}

void EngineMain::RecordSceneDraws(std::shared_ptr<DXGraphicsContext> context)
{
    m_world->PreGatherDrawCalls(context);
    context->ApplyLightBuffersToCommandList();
    m_world->GatherDrawCalls(context);
}

void EngineMain::Cleanup()
{
    m_cameraGameObject.reset();
    if (m_world)
    {
        m_world->Clear();
        m_world.reset();
    }

    if (dxRenderManager)
    {
        dxRenderManager->OnDestroy();
        dxRenderManager.reset();
    }
}
