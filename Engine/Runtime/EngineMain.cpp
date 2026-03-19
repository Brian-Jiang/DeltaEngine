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
#include "Core/DScene.h"
#include "Assets/PA_DScene.h"
#include "Assets/DPrimaryAsset.h"
#include "Assets/AssetDatabaseLocator.h"
#include "Reflection/ReflectionRegistry.h"
#include "Reflection/DFunction.h"
#include "Reflection/DClass.h"
#include "IO/IOManager.h"

#include "Runtime/Test/TestComponent.h"

#define SCREEN_WIDTH   1280
#define SCREEN_HEIGHT  720

using namespace DeltaEngine;
using namespace DirectX;

EngineMain::EngineMain()
    : exitCode(0)
    , gameState(GameState::PLAY)
    , m_cameraGameObject(nullptr)
{
    time = std::make_unique<Time>();
    GetReflectionRegistry().FinalizeRegistration();
}

DWorld* EngineMain::GetWorld() const
{
    for (const WorldContext& ctx : m_worldContextList)
    {
        if (ctx.type == WorldType::Editor && ctx.world)
            return ctx.world;
    }
    return nullptr;
}

void EngineMain::Initialize(std::shared_ptr<DXRenderManager> sceneRenderer, std::shared_ptr<SDL_Window> window)
{
    dxRenderManager = std::move(sceneRenderer);
    m_window = window;

    //DWorld* world = GetWorld();

    //// ---------- game object: star mesh renderer
    //{
    //    GameObject* go = world->CreateGameObject("MeshRenderer");
    //    MeshRenderer* meshRenderer = go->AddSceneComponent<MeshRenderer>();
    //    meshRenderer->SetLocalPosition(2.0f, 0.0f, 5.0f);

    //    DShader* shader = CreateDObject<DShader>();
    //    shader->Initialize(
    //        L"Shaders.hlsl",
    //        L"VSMain", L"PSMain",
    //        L"vs_6_0", L"ps_6_0");

    //    shader->SetInputLayout({
    //        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //    });

    //    //std::shared_ptr<DMaterial> material = std::make_shared<DMaterial>(shader);

    //    DMesh* mesh = CreateDObject<DMesh>();
    //    mesh->Initialize(std::wstring(L"Star.obj"));
    //    std::vector<DTexture*> textures = mesh->GetTextures();
    //    std::vector<DMaterial*> materials;
    //    for (int i = 0; i < mesh->GetSubMeshCount(); i++)
    //    {
    //        DMaterial* material = CreateDObject<DMaterial>();
    //        material->Initialize(shader);
    //        if (i < textures.size())
    //        {
    //            material->AddTexture(textures[i]);
    //        }
    //        materials.push_back(material);
    //    }

    //    mesh->SetMaterials(materials);
    //    meshRenderer->SetMesh(mesh);
    //}

    //// ---------- game object: home mesh renderer
    //{
    //    GameObject* go = world->CreateGameObject("HomeMeshRenderer");
    //    MeshRenderer* meshRenderer = go->AddSceneComponent<MeshRenderer>();
    //    meshRenderer->SetLocalPosition(0.0f, 0.0f, 0.0f);

    //    DShader* shader = CreateDObject<DShader>();
    //    shader->Initialize(
    //        L"ToonShader.hlsl",
    //        L"VSMain", L"PSMain",
    //        L"vs_6_0", L"ps_6_0");

    //    shader->SetInputLayout({
    //        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //    });
    //    //std::shared_ptr<DMaterial> material = std::make_shared<DMaterial>(shader);

    //    DMesh* mesh = CreateDObject<DMesh>();
    //    mesh->Initialize(std::wstring(L"home/source/home.fbx"));
    //    //std::shared_ptr<DMesh> mesh = std::make_shared<DMesh>(std::wstring(L"car/source/datsun240k.fbx"));
    //    std::vector<DTexture*> textures = mesh->GetTextures();
    //    std::vector<DMaterial*> materials;
    //    for (int i = 0; i < mesh->GetSubMeshCount(); i++)
    //    {
    //        DMaterial* material = CreateDObject<DMaterial>();
    //        material->Initialize(shader);
    //        if (i < textures.size())
    //        {
    //            material->AddTexture(textures[i]);
    //        }
    //        materials.push_back(material);
    //    }

    //    mesh->SetMaterials(materials);
    //    meshRenderer->SetMesh(mesh);
    //}


    //// ---------- game object: camera
    //GameObject* cameraGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "Camera");
    //m_cameraGameObject = cameraGo;
    //Camera* camera = cameraGo->AddSceneComponent<Camera>();
    //camera->SetLocalPosition(0.0f, 50.0f, -400.0f);
    //camera->UpdateParameters(DirectX::XM_PIDIV4, static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT, 0.1f, 1000.0f);


    //// ---------- game object: directional light (sun)
    //GameObject* directionalLightGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "DirectionalLight");
    //DirectionalLight* directionalLight = directionalLightGo->AddSceneComponent<DirectionalLight>();
    //directionalLight->SetLocalRotation(DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitX, XM_PIDIV4));
    //directionalLight->UpdateParameters(XMVectorSet(0, -1, 0, 0), XMVectorSet(1.0f, 1.0f, 0.95f, 1.0f), 0.7f);

    //// ---------- game object: point light
    //GameObject* pointLightGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "PointLight");
    //PointLight* pointLight = pointLightGo->AddSceneComponent<PointLight>();
    //pointLight->SetLocalPosition(0.0f, 3.0f, 2.0f);
    //pointLight->UpdateParameters(XMVectorSet(1.0f, 0.4f, 0.2f, 1.0f), 2.0f, 15.0f);

    //// ---------- game object: spot light
    //GameObject* spotLightGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "SpotLight");
    //SpotLight* spotLight = spotLightGo->AddSceneComponent<SpotLight>();
    //spotLight->SetLocalPosition(-2.0f, 4.0f, 0.0f);
    //spotLight->SetLocalRotation(DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitZ, -XM_PIDIV4));
    //spotLight->UpdateParameters(XMVectorSet(0.2f, 0.8f, 1.0f, 1.0f), 3.0f, 20.0f, XM_PI / 6.0f, XM_PI / 3.0f);
    //

    ////TestComponent* testComp = CreateDObject<TestComponent>();
    //cameraGo->AddComponent<TestComponent>();

    //DClass* testClass = GetReflectionRegistry().FindClassByName("TestComponent");

    //DFunction* addFn = testClass->FindFunctionByName("TestAdd");
    ////struct { int a; int b; int retVal; } addParams = { 3, 7, 0 };
    //struct TestAdd_Params {
    //    int a;
    //    int b;
    //    int returnValue;
    //} addParams = { 3, 7, 0 };
    //addFn->Invoke(testComp, &addParams);
    //int addResult = addParams.returnValue;

    //DFunction* mulFn = testClass->FindFunctionByName("TestMultiply");
    ////struct { float x; bool negate; char _pad[3]; float retVal; } mulParams = { 5.0f, true, {}, 0.0f };
    //struct TestMultiply_Params {
    //    float x;
    //    bool neg;
    //    float returnValue;
    //} mulParams2 = { 5.0f, true, 0.0f };
    //mulFn->Invoke(testComp, &mulParams2);
    //float mulResult = mulParams2.returnValue;

    //DClass* testClass2 = GetReflectionRegistry().FindClassByName("TestComponent2");


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


    //dxRenderManager->InitWorldRenderers(*world);

    //atexit(&Device::ReportLiveObjects);
}

void DeltaEngine::EngineMain::PreTick()
{
    if (DWorld* world = GetWorld())
        world->PreTick(Time::deltaTime);
}

void EngineMain::Tick()
{
    time->TickTime();
}

Camera* EngineMain::GetCamera()
{
    if (!m_cameraGameObject)
        return nullptr;

    return m_cameraGameObject->GetRootSceneComponent<Camera>();
}

void EngineMain::ProcessEvent(const SDL_Event& event)
{
    // Camera movement (WASD, Q/E) is handled by EditorWindow_Viewport when right-dragging on viewport.
}

void EngineMain::OnWindowResized(UINT width, UINT height)
{
    if (m_cameraGameObject)
        m_cameraGameObject->GetRootSceneComponent<Camera>()->UpdateAspectRatio(static_cast<float>(width) / height);
}

void EngineMain::RecordSceneDraws(std::shared_ptr<DXGraphicsContext> context)
{
    if (DWorld* world = GetWorld())
    {
        world->PreGatherDrawCalls(context);
        context->ApplyLightBuffersToCommandList();
        world->GatherDrawCalls(context);
    }
}

void EngineMain::CreateWorld()
{
    // ---- Build the editor world ----
    DWorld* world = CreateDObject<DWorld>();
    m_worldContextList.push_back(WorldContext { WorldType::Editor, world });
}

void EngineMain::CreateGameObjects()
{
    DWorld* world = GetWorld();
    IAssetDatabase& assetDb = AssetDatabaseLocator::Get();

    // ---------- game object: star mesh renderer
    {
        GameObject* go = world->CreateGameObject("MeshRenderer");
        MeshRenderer* meshRenderer = go->AddSceneComponent<MeshRenderer>();
        meshRenderer->SetLocalPosition(2.0f, 0.0f, 5.0f);

        //const std::filesystem::path shaderPath = IOManager::GetEngineImportedAssetFullPath("StarShader");
        //DPrimaryAsset* shaderAsset = assetDb.LoadAsset(assetDb.FindAssetIdByPath(shaderPath));
        //DShader* shader = shaderAsset->GetObjects().empty() ? nullptr : dynamic_cast<DShader*>(shaderAsset->GetObjects()[0]);

        DShader* shader = CreateDObject<DShader>();
        shader->Initialize(
            L"Shaders.hlsl",
            L"VSMain", L"PSMain",
            L"vs_6_0", L"ps_6_0");

        shader->SetInputLayout({
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        });

        // std::shared_ptr<DMaterial> material = std::make_shared<DMaterial>(shader);

        //const std::filesystem::path meshPath = IOManager::GetEngineImportedAssetFullPath("StarMesh");
        //DPrimaryAsset* meshAsset = assetDb.LoadAsset(assetDb.FindAssetIdByPath(meshPath));
        //DMesh* mesh = meshAsset->GetObjects().empty() ? nullptr : dynamic_cast<DMesh*>(meshAsset->GetObjects()[0]);

        DMesh* mesh = CreateDObject<DMesh>();
        mesh->Initialize(std::wstring(L"Star.obj"));
        std::vector<DTexture*> textures = mesh->GetTextures();
        std::vector<DMaterial*> materials;
        for (int i = 0; i < mesh->GetSubMeshCount(); i++) {
            DMaterial* material = CreateDObject<DMaterial>();
            material->Initialize(shader);
            if (i < textures.size()) {
                material->AddTexture(textures[i]);
            }
            materials.push_back(material);
        }

        mesh->SetMaterials(materials);
        meshRenderer->SetMesh(mesh);
    }

    // ---------- game object: home mesh renderer
    {
        GameObject* go = world->CreateGameObject("HomeMeshRenderer");
        MeshRenderer* meshRenderer = go->AddSceneComponent<MeshRenderer>();
        meshRenderer->SetLocalPosition(0.0f, 0.0f, 0.0f);

        DShader* shader = CreateDObject<DShader>();
        shader->Initialize(
            L"ToonShader.hlsl",
            L"VSMain", L"PSMain",
            L"vs_6_0", L"ps_6_0");

        shader->SetInputLayout({
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        });
        // std::shared_ptr<DMaterial> material = std::make_shared<DMaterial>(shader);

        DMesh* mesh = CreateDObject<DMesh>();
        mesh->Initialize(std::wstring(L"home/source/home.fbx"));
        // std::shared_ptr<DMesh> mesh = std::make_shared<DMesh>(std::wstring(L"car/source/datsun240k.fbx"));
        std::vector<DTexture*> textures = mesh->GetTextures();
        std::vector<DMaterial*> materials;
        for (int i = 0; i < mesh->GetSubMeshCount(); i++) {
            DMaterial* material = CreateDObject<DMaterial>();
            material->Initialize(shader);
            if (i < textures.size()) {
                material->AddTexture(textures[i]);
            }
            materials.push_back(material);
        }

        mesh->SetMaterials(materials);
        meshRenderer->SetMesh(mesh);
    }

    // ---------- game object: camera
    GameObject* cameraGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "Camera");
    m_cameraGameObject = cameraGo;
    Camera* camera = cameraGo->AddSceneComponent<Camera>();
    camera->SetLocalPosition(0.0f, 50.0f, -400.0f);
    camera->UpdateParameters(DirectX::XM_PIDIV4, static_cast<float>(SCREEN_WIDTH) / SCREEN_HEIGHT, 0.1f, 1000.0f);

    // ---------- game object: directional light (sun)
    GameObject* directionalLightGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "DirectionalLight");
    DirectionalLight* directionalLight = directionalLightGo->AddSceneComponent<DirectionalLight>();
    directionalLight->SetLocalRotation(DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitX, XM_PIDIV4));
    directionalLight->UpdateParameters(XMVectorSet(0, -1, 0, 0), XMVectorSet(1.0f, 1.0f, 0.95f, 1.0f), 0.7f);

    // ---------- game object: point light
    GameObject* pointLightGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "PointLight");
    PointLight* pointLight = pointLightGo->AddSceneComponent<PointLight>();
    pointLight->SetLocalPosition(0.0f, 3.0f, 2.0f);
    pointLight->UpdateParameters(XMVectorSet(1.0f, 0.4f, 0.2f, 1.0f), 2.0f, 15.0f);

    // ---------- game object: spot light
    GameObject* spotLightGo = world->CreateGameObjectInScene(m_worldContextList[0].world->GetActiveScene(), "SpotLight");
    SpotLight* spotLight = spotLightGo->AddSceneComponent<SpotLight>();
    spotLight->SetLocalPosition(-2.0f, 4.0f, 0.0f);
    spotLight->SetLocalRotation(DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(DirectX::SimpleMath::Vector3::UnitZ, -XM_PIDIV4));
    spotLight->UpdateParameters(XMVectorSet(0.2f, 0.8f, 1.0f, 1.0f), 3.0f, 20.0f, XM_PI / 6.0f, XM_PI / 3.0f);

    // TestComponent* testComp = CreateDObject<TestComponent>();
    cameraGo->AddComponent<TestComponent>();

    dxRenderManager->InitWorldRenderers(*world);
}

void EngineMain::LoadScene(const AssetId& sceneAssetId)
{
    DPrimaryAsset* asset = AssetDatabaseLocator::Get().LoadAsset(sceneAssetId);
    if (!asset)
        return;

    DScene* scene = nullptr;
    if (PA_DScene* sceneAsset = dynamic_cast<PA_DScene*>(asset))
        scene = sceneAsset->GetScene();

    //if (!scene)
    //{
    //    for (DObject* object : asset->GetObjects())
    //    {
    //        if (DScene* typedScene = dynamic_cast<DScene*>(object))
    //        {
    //            scene = typedScene;
    //            break;
    //        }
    //    }
    //}

    if (!scene)
        return;

    DWorld* world = GetWorld();
    if (!world)
        return;

    for (GameObject* go : scene->GetGameObjects())
        world->AddGameObjectFromScene(go);

    // First scene loaded becomes the active scene.
    if (!world->GetActiveScene())
        world->SetActiveScene(scene);

    // Re-initialise GPU state for any newly added renderers.
    dxRenderManager->InitWorldRenderers(*world);
}

void EngineMain::Cleanup()
{
    m_cameraGameObject = nullptr;
    if (DWorld* world = GetWorld())
        world->Clear();

    m_worldContextList.clear();

    if (dxRenderManager)
    {
        dxRenderManager->OnDestroy();
        dxRenderManager.reset();
    }
}
