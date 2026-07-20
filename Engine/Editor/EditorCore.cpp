#include "EditorCore.h"

#include "Editor/Animation/EditorAnimationManager.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/EditorCommand.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommandRegistry.h"
#include "Editor/Commands/EditorAuxiliarySceneCommands.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"
#include "Editor/Commands/EditorCommand_RenameObject.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/EditorSessionState.h"
#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Graphics/Light/LightComponent.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/EngineMain.h"
#include "Runtime/IO/IOManager.h"
#include "Editor/Mcp/McpProtocol.h"
#include "Editor/Mcp/McpQueryRouter.h"
#include "Editor/Mcp/McpRegistry.h"
#include "Editor/McpSocketServer.h"
#include "Runtime/Assets/PA_CommonAssets.h"
#include "Runtime/Core/DTexture.h"
#include "Runtime/Core/DShader.h"
#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/Skybox.h"
#include "Runtime/Graphics/PostProcess/PA_PostProcessStack.h"
#include "Runtime/Core/DMesh.h"

#include <DirectXMath.h>
#include <SimpleMath.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdio>
#include <memory>

using namespace DeltaEngine;

DEFINE_LOG_CATEGORY(DeltaEngine::LogEditorCore)

EditorCore* DeltaEngine::g_editorCore = nullptr;
std::unique_ptr<McpSocketServer> DeltaEngine::g_mcpServer;

EditorCore::EditorCore()
{
    g_editorCore = this;
}

EditorCore::~EditorCore()
{
    Shutdown();
    g_editorCore = nullptr;
}

void EditorCore::Initialize(EngineMain& engine, bool headless, std::filesystem::path assetRootOverride)
{
    DELTA_VERIFY_MSG(m_engine == nullptr, "EditorCore::Initialize called twice without Shutdown");
    m_engine = &engine;
    m_headless = headless;

    DLOG(LogEditorCore, ELogLevel::Verbose,
         "EditorCore::Initialize headless={} assetRootOverride='{}'",
         headless, assetRootOverride.string());

    m_assetDatabase = std::make_unique<EditorAssetDatabase>();
    AssetDatabaseLocator::Register(m_assetDatabase.get());

    CreateAssets();

    if (!assetRootOverride.empty())
    {
        m_assetRoot = std::filesystem::weakly_canonical(assetRootOverride);
        PA_DScene* defaultScene = PA_DScene::Create("DefaultScene");
        m_assetDatabase->CreateAsset(m_assetRoot / "DefaultScene.dasset.json", defaultScene);

        m_assetDatabase->ScanAssetsFolder(m_assetRoot);

        PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(
            m_assetDatabase->FindAssetIdByPath(m_assetRoot / "DefaultScene.dasset.json"));
        if (sceneAsset)
            m_engine->LoadScene(sceneAsset->GetAssetId());
        else
            DLOG(LogEditorCore, ELogLevel::Error,
                 "Failed to load DefaultScene (assetRootOverride='{}'); expected DefaultScene.dasset.json under that root",
                 assetRootOverride.string());
    }
    else
    {
        m_assetRoot = std::filesystem::weakly_canonical(
            std::filesystem::path(IOManager::GetEngineImportedAssetsFolder()));
        m_assetDatabase->ScanAssetsFolder(m_assetRoot);

        AssetId sceneId;
        const EditorSessionState session = LoadEditorSessionState();
        if (!session.lastScenePath.empty())
        {
            const std::filesystem::path canonical =
                std::filesystem::weakly_canonical(std::filesystem::path(session.lastScenePath));
            sceneId = m_assetDatabase->FindAssetIdByPath(canonical);
            if (sceneId.IsNull())
                DLOG(LogEditorCore, ELogLevel::Log,
                     "Last opened scene '{}' not found in asset database; falling back to DefaultScene",
                     session.lastScenePath);
        }

        if (sceneId.IsNull())
            sceneId = m_assetDatabase->FindAssetIdByPath(
                IOManager::GetEngineImportedAssetFullPath("DefaultScene"));

        PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(sceneId);
        if (sceneAsset)
            m_engine->LoadScene(sceneAsset->GetAssetId());
        else
            DLOG(LogEditorCore, ELogLevel::Error,
                 "Failed to load startup scene from imported assets folder '{}'; expected DefaultScene.dasset.json",
                 IOManager::GetEngineImportedAssetsFolder().string());
    }

    m_selectionState = std::make_unique<EditorSelectionState>();
    m_commandManager = std::make_unique<EditorCommandManager>();

    if (!m_headless)
        m_animationManager = std::make_unique<EditorAnimationManager>();

    m_mcpRegistry = std::make_unique<McpRegistry>();
    m_mcpRegistry->InitializeAll(*this);

    m_mcpRouter = std::make_shared<McpQueryRouter>(*this, *m_mcpRegistry);

    if (!m_headless)
    {
        constexpr uint16_t kPreferredMcpPort = 57340;
        // Socket-thread handler: hand the raw line to the main thread and block for
        // the result. Both query and command lines take the same path.
        auto handler = [this](const std::string& json) -> std::string {
            constexpr auto kMcpRequestTimeout = std::chrono::seconds(30);
            std::future<std::string> fut = SubmitMcpRequest(json);
            if (fut.wait_for(kMcpRequestTimeout) != std::future_status::ready)
                return nlohmann::json{{"ok", false},
                                      {"error", "timed out waiting for main-thread execution"}}
                    .dump();
            return fut.get();
        };
        g_mcpServer = std::make_unique<McpSocketServer>(handler, handler);
        const uint16_t mcpPort = g_mcpServer->Start(kPreferredMcpPort);
        if (mcpPort == 0)
            DLOG(LogEditorCore,
                 ELogLevel::Error,
                 "MCP socket server failed to bind (tried up to 10 sequential ports beginning at {})",
                 static_cast<unsigned>(kPreferredMcpPort));
    }
}

void EditorCore::Shutdown()
{
    DLOG(LogEditorCore, ELogLevel::Verbose, "EditorCore::Shutdown");
    // Unblock any socket thread waiting on a pending request before we join the
    // server thread, otherwise its future.get() could deadlock the join.
    CancelPendingMcpRequests();
    g_mcpServer.reset();
    m_mcpRouter.reset();
    m_mcpRegistry.reset();
    m_animationManager.reset();
    if (m_commandManager)
    {
        m_commandManager->Clear();
        m_commandManager.reset();
    }
    m_selectionState.reset();
    if (m_assetDatabase)
    {
        AssetDatabaseLocator::Unregister();
        m_assetDatabase.reset();
    }
    m_engine = nullptr;
}

void EditorCore::SetTestValue(const std::string& key, const std::string& value)
{
    m_testValueStore[key] = value;
}

std::string EditorCore::GetTestValue(const std::string& key) const
{
    auto it = m_testValueStore.find(key);
    return it != m_testValueStore.end() ? it->second : std::string{};
}

DWorld* EditorCore::GetWorld()
{
    return m_engine ? m_engine->GetWorld() : nullptr;
}

void EditorCore::LoadScene(const std::filesystem::path& scenePath)
{
    if (!DELTA_ENSURE(m_assetDatabase && m_engine))
    {
        DLOG(LogEditorCore, ELogLevel::Error,
             "LoadScene called before Initialize (path='{}'); expected initialized EditorCore",
             scenePath.string());
        return;
    }

    DLOG(LogEditorCore, ELogLevel::Verbose, "LoadScene: path='{}'", scenePath.string());

    const std::filesystem::path canonical = std::filesystem::weakly_canonical(scenePath);
    AssetId id = m_assetDatabase->FindAssetIdByPath(canonical);
    if (id.IsNull())
    {
        DLOG(LogEditorCore, ELogLevel::Warning,
             "LoadScene: scene asset not found for path '{}' (canonical='{}'); expected a registered .dasset.json",
             scenePath.string(), canonical.string());
        return;
    }

    if (DWorld* world = m_engine->GetWorld())
        world->DetachAllWorldGameObjects();

    if (m_selectionState)
        m_selectionState->ClearAll();

    m_assetDatabase->ReloadAssetFromDisk(id);

    PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(id);
    if (sceneAsset)
    {
        m_engine->LoadScene(sceneAsset->GetAssetId());
        if (!m_headless)
        {
            EditorSessionState session;
            session.lastScenePath = canonical.string();
            SaveEditorSessionState(session);
        }
    }
}

DObject* EditorCore::ResolveObject(const AssetId& assetId, const ObjectId& objectId)
{
    if (!m_assetDatabase || assetId.IsNull() || objectId.IsNull())
        return nullptr;

    DPrimaryAsset* asset = m_assetDatabase->GetLoadedAsset(assetId);
    if (!asset)
        asset = m_assetDatabase->LoadAsset(assetId);
    if (!asset)
    {
        DLOG(LogEditorCore, ELogLevel::Warning,
             "ResolveObject: asset not found or failed to load (assetId='{}'); expected a registered, loadable asset",
             assetId.ToString());
        return nullptr;
    }

    DObject* obj = asset->FindObject(objectId);
    if (!obj)
    {
        DLOG(LogEditorCore, ELogLevel::Warning,
             "ResolveObject: objectId '{}' not found in asset '{}' (expected an object owned by this asset)",
             objectId.ToString(), assetId.ToString());
    }
    return obj;
}

// Only searches assets already loaded in memory. If you have an AssetId, use ResolveObject for command paths.
std::pair<AssetId, ObjectId> EditorCore::GetIdsForObject(DObject* obj)
{
    if (!obj || !m_assetDatabase)
        return {AssetId::Null(), ObjectId::Null()};

    for (const auto& [id, entry] : m_assetDatabase->GetAllAssets())
    {
        DPrimaryAsset* asset = entry.m_instance.Get();
        if (!asset)
            continue;

        for (DObject* o : asset->GetObjects())
        {
            if (o == obj)
                return {asset->GetAssetId(), obj->GetObjectId()};
        }
    }

    return {AssetId::Null(), ObjectId::Null()};
}

void EditorCore::NotifyObjectDestroyed(const ObjectId& objectId)
{
    if (m_selectionState)
        m_selectionState->NotifyObjectDestroyed(objectId);
}

DPrimaryAsset* EditorCore::GetActiveSceneAsset()
{
    DWorld* world = GetWorld();
    if (!world) return nullptr;
    DScene* scene = world->GetActiveScene();
    if (!scene) return nullptr;
    return scene->GetOwningAsset();
}

std::future<std::string> EditorCore::SubmitMcpRequest(std::string json)
{
    std::promise<std::string> promise;
    std::future<std::string> future = promise.get_future();

    std::lock_guard lock(m_mcpRequestMutex);
    if (m_mcpShuttingDown)
    {
        promise.set_value(nlohmann::json{{"ok", false}, {"error", "editor shutting down"}}.dump());
        return future;
    }
    m_mcpRequestQueue.push_back({std::move(json), std::move(promise)});
    return future;
}

void EditorCore::DrainMcpRequests()
{
    std::deque<PendingMcpRequest> batch;
    {
        std::lock_guard lock(m_mcpRequestMutex);
        batch.swap(m_mcpRequestQueue);
    }

    for (auto& req : batch)
    {
        std::string response;
        try
        {
            response = m_mcpRouter ? m_mcpRouter->Route(req.json)
                                   : nlohmann::json{{"ok", false}, {"error", "no MCP router"}}.dump();
        }
        catch (const std::exception& e)
        {
            response = nlohmann::json{{"ok", false}, {"error", e.what()}}.dump();
        }
        req.result.set_value(std::move(response));
    }
}

void EditorCore::CancelPendingMcpRequests()
{
    std::deque<PendingMcpRequest> batch;
    {
        std::lock_guard lock(m_mcpRequestMutex);
        m_mcpShuttingDown = true;
        batch.swap(m_mcpRequestQueue);
    }

    for (auto& req : batch)
        req.result.set_value(nlohmann::json{{"ok", false}, {"error", "editor shutting down"}}.dump());
}

 void EditorCore::CreateAssets()
 {
     // Run once to author the default plane asset, then comment out.
     //{
     //    constexpr float kHalf = 500.0f; // 1000x1000 units, centered at origin on the XZ plane
     //    const DirectX::XMFLOAT4 white { 1.0f, 1.0f, 1.0f, 1.0f };
     //    const DirectX::XMFLOAT3 up { 0.0f, 1.0f, 0.0f };
     //    const DirectX::XMFLOAT3 tangent { 1.0f, 0.0f, 0.0f };

     //    std::vector<Vertex> vertices {
     //        { { -kHalf, 0.0f, -kHalf }, white, up, tangent, { 0.0f, 0.0f } },
     //        { { -kHalf, 0.0f,  kHalf }, white, up, tangent, { 0.0f, 1.0f } },
     //        { {  kHalf, 0.0f,  kHalf }, white, up, tangent, { 1.0f, 1.0f } },
     //        { {  kHalf, 0.0f, -kHalf }, white, up, tangent, { 1.0f, 0.0f } },
     //    };
     //    std::vector<unsigned int> indices { 0, 1, 2, 0, 2, 3 };

     //    DMesh* mesh = CreateDObject<DMesh>();
     //    mesh->SetGeometry(std::move(vertices), std::move(indices));

     //    DMaterial* material = CreateDObject<DMaterial>();
     //    m_assetDatabase->CreateAsset(
     //        IOManager::GetEngineImportedAssetFullPath("PlaneMaterial"),
     //        PA_Material::Create(material));

     //    std::vector<DMaterial*> materials { material };
     //    mesh->SetMaterials(materials);

     //    m_assetDatabase->CreateAsset(
     //        IOManager::GetEngineImportedAssetFullPath("PlaneMesh"),
     //        PA_StaticMesh::Create(mesh));
     //}

     //{
     //    DMesh *mesh = CreateDObject<DMesh>();
     //    mesh->Initialize(std::wstring(L"sphere.fbx"));
     //    //DMaterial *material = CreateDObject<DMaterial>();
     //    //material->Initialize(nullptr);
     //    //m_assetDatabase->CreateAsset(
     //    //        IOManager::GetEngineImportedAssetFullPath("SphereMaterial_0"),
     //    //        PA_Material::Create(material));
     //    //std::vector<DMaterial *> materials{material};
     //    //mesh->SetMaterials(materials);
     //    m_assetDatabase->CreateAsset(
     //            IOManager::GetEngineImportedAssetFullPath("SphereMesh"),
     //            PA_StaticMesh::Create(mesh));
     //}

    //{
    //    DShader *shader = CreateDObject<DShader>();
    //    {
    //        shader->Initialize(
    //            L"Shaders/PBRObject.slang",
    //            L"VSMain", L"PSMain",
    //            L"vs_6_6", L"ps_6_6");
    //     
    //        shader->SetInputLayout({
    //            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    //        });
    //     
    //        m_assetDatabase->CreateAsset(
    //            IOManager::GetEngineImportedAssetFullPath("PBRShader"),
    //            PA_Shader::Create(shader));
    //    }
    //}

     //{
     //    const std::filesystem::path psShaderPath = IOManager::GetEngineImportedAssetFullPath("PassthroughShader");
     //    DShader *psShader = CreateDObject<DShader>();
     //    psShader->Initialize("Shaders/PostProcess_Passthrough.hlsl", "VSMain", "PSMain", "vs_6_0", "ps_6_0");
     //    PA_Shader *psShaderPA = PA_Shader::Create(psShader);
     //    m_assetDatabase->CreateAsset(psShaderPath, psShaderPA);
     //}

     //{
     //    const std::filesystem::path psShaderPath = IOManager::GetEngineImportedAssetFullPath("TonemapShader");
     //    DShader *psShader = CreateDObject<DShader>();
     //    psShader->Initialize("Shaders/PostProcess_Tonemap.hlsl", "VSMain", "PSMain", "vs_6_0", "ps_6_0");
     //    PA_Shader *psShaderPA = PA_Shader::Create(psShader);
     //    m_assetDatabase->CreateAsset(psShaderPath, psShaderPA);
     //}

     //{
     //    const std::filesystem::path psShaderPath = IOManager::GetEngineImportedAssetFullPath("BloomShader");
     //    DShader *psShader = CreateDObject<DShader>();
     //    psShader->Initialize("Shaders/PostProcess_Bloom.hlsl", "VSMain", "PSMain", "vs_6_0", "ps_6_0");
     //    PA_Shader *psShaderPA = PA_Shader::Create(psShader);
     //    m_assetDatabase->CreateAsset(psShaderPath, psShaderPA);
     //}

     // TODO use shader assets?
     //const std::filesystem::path ppStackPath = IOManager::GetEngineImportedAssetFullPath("PostProcess/PostProcessStack");
     //PA_PostProcessStack *ppStack = PA_PostProcessStack::Create();
     //ppStack->AddPass("PassthroughPass");
     //ppStack->AddPass("TonemapPass");
     //ppStack->AddPass("BloomPass");
     //m_assetDatabase->CreateAsset(ppStackPath, ppStack);

     //const std::filesystem::path skyboxTexturePath = IOManager::GetEngineImportedAssetFullPath("SkyboxTexture");
     //const std::filesystem::path skyboxTextureSourcePath = IOManager::GetEngineSourceAssetFullPath(L"SkyboxCubemap.dds");
     //PA_Texture *skyboxTexture = PA_Texture::Create(DTexture::LoadFromFile(skyboxTextureSourcePath));
     //m_assetDatabase->CreateAsset(skyboxTexturePath, skyboxTexture);

     //const std::filesystem::path skyboxMaterialPath = IOManager::GetEngineImportedAssetFullPath("SkyboxMaterial");
     // 
     //const std::filesystem::path skyboxShaderPath = IOManager::GetEngineImportedAssetFullPath("SkyboxShader");
     //DShader *shader = CreateDObject<DShader>();
     //shader->Initialize(L"Skybox.hlsl", L"VSMain", L"PSMain", L"vs_6_0", L"ps_6_0");
     //PA_Shader *shaderAsset = PA_Shader::Create(shader);
     //m_assetDatabase->CreateAsset(skyboxShaderPath, shaderAsset);

     //DMaterial* material = CreateDObject<DMaterial>();
     //material->Initialize(shader);
     //
     //PA_Material *materialAsset = PA_Material::Create(material);
     //m_assetDatabase->CreateAsset(skyboxMaterialPath, materialAsset);

     //const std::filesystem::path skyboxAssetPath = IOManager::GetEngineImportedAssetFullPath("DefaultSkybox");
     //Skybox *skybox = CreateDObject<Skybox>();
     //PA_Skybox *skyboxAsset = PA_Skybox::Create(skybox);
     //m_assetDatabase->CreateAsset(skyboxAssetPath, skyboxAsset);

     //     // Default scene
//     const std::filesystem::path scenePath = IOManager::GetEngineImportedAssetFullPath("DefaultScene");
//
//     PA_DScene* sceneAsset = PA_DScene::Create("DefaultScene");
//     m_assetDatabase->CreateAsset(scenePath, sceneAsset);
//     AssetId sceneId = sceneAsset->GetAssetId();
//
//     // Default shader
//     DShader* shader = CreateDObject<DShader>();
//     {
//         shader->Initialize(
//             L"Shaders.hlsl",
//             L"VSMain", L"PSMain",
//             L"vs_6_0", L"ps_6_0");
//
//         shader->SetInputLayout({
//             { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//             { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//             { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//             { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//         });
//
//         m_assetDatabase->CreateAsset(
//             IOManager::GetEngineImportedAssetFullPath("DefaultShader"),
//             PA_Shader::Create(shader));
//     }
//
     //// Star mesh and material
     //{
     //    DMesh* mesh = CreateDObject<DMesh>();
     //    mesh->Initialize(std::wstring(L"Star.obj"));
     //    std::vector<DTexture*> textures = mesh->GetTextures();
     //    std::vector<DMaterial*> materials;
     //    for (int i = 0; i < mesh->GetSubMeshCount(); i++) {
     //        DMaterial* material = CreateDObject<DMaterial>();
     //        material->Initialize(nullptr);
     //        if (i < textures.size()) {
     //            m_assetDatabase->CreateAsset(
     //                IOManager::GetEngineImportedAssetFullPath("StarTexture_" + std::to_string(i)),
     //                PA_Texture::Create(textures[i]));
     //            material->AddTexture(textures[i]);
     //        }
     //        materials.push_back(material);

     //        m_assetDatabase->CreateAsset(
     //            IOManager::GetEngineImportedAssetFullPath("StarMaterial_" + std::to_string(i)),
     //            PA_Material::Create(material));
     //    }

     //    mesh->SetMaterials(materials);

     //    m_assetDatabase->CreateAsset(
     //        IOManager::GetEngineImportedAssetFullPath("StarMesh"),
     //        PA_StaticMesh::Create(mesh));
     //}

     //// Home mesh and material
     //{
     //    DMesh* mesh = CreateDObject<DMesh>();
     //    mesh->Initialize(std::wstring(L"home/source/home.fbx"));
     //    std::vector<DTexture*> textures = mesh->GetTextures();
     //    std::vector<DMaterial*> materials;
     //    for (int i = 0; i < mesh->GetSubMeshCount(); i++) {
     //        DMaterial* material = CreateDObject<DMaterial>();
     //        material->Initialize(nullptr);
     //        if (i < textures.size()) {
     //            m_assetDatabase->CreateAsset(
     //                IOManager::GetEngineImportedAssetFullPath("HomeTexture_" + std::to_string(i)),
     //                PA_Texture::Create(textures[i]));
     //            material->SetAlbedoTexture(textures[i]);
     //        }
     //        materials.push_back(material);

     //        m_assetDatabase->CreateAsset(
     //            IOManager::GetEngineImportedAssetFullPath("HomeMaterial_" + std::to_string(i)),
     //            PA_Material::Create(material));
     //    }

     //    mesh->SetMaterials(materials);

     //    m_assetDatabase->CreateAsset(
     //        IOManager::GetEngineImportedAssetFullPath("HomeMesh"),
     //        PA_StaticMesh::Create(mesh));
     //}
}
