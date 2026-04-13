#include "EditorCore.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/EditorCommand.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommandRegistry.h"
#include "Editor/EditorSelectionState.h"
#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/EngineMain.h"
#include "Runtime/IO/IOManager.h"
#include <nlohmann/json.hpp>

#include <cstdio>

using namespace DeltaEngine;

DEFINE_LOG_CATEGORY(DeltaEngine::LogEditorCore)

EditorCore* DeltaEngine::g_editorCore = nullptr;

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
    m_engine = &engine;
    m_headless = headless;

    m_assetDatabase = std::make_unique<EditorAssetDatabase>();
    AssetDatabaseLocator::Register(m_assetDatabase.get());

    if (!assetRootOverride.empty())
    {
        const std::filesystem::path root = std::filesystem::weakly_canonical(assetRootOverride);
        PA_DScene* defaultScene = PA_DScene::Create("DefaultScene");
        m_assetDatabase->CreateAsset(root / "DefaultScene.dasset.json", defaultScene);

        m_assetDatabase->ScanAssetsFolder(root);

        PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(
            m_assetDatabase->FindAssetIdByPath(root / "DefaultScene.dasset.json"));
        if (sceneAsset)
            m_engine->LoadScene(sceneAsset->GetAssetId());
        else
            std::printf("Failed to load DefaultScene (temp root).\n");
    }
    else
    {
        m_assetDatabase->ScanAssetsFolder(IOManager::GetEngineImportedAssetsFolder());

        PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(
            m_assetDatabase->FindAssetIdByPath(IOManager::GetEngineImportedAssetFullPath("DefaultScene")));
        if (sceneAsset)
            m_engine->LoadScene(sceneAsset->GetAssetId());
        else
            std::printf("Failed to load DefaultScene.\n");
    }

    m_selectionState = std::make_unique<EditorSelectionState>();
    m_commandManager = std::make_unique<EditorCommandManager>();
}

void EditorCore::Shutdown()
{
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
    if (!m_assetDatabase || !m_engine)
        return;

    const std::filesystem::path canonical = std::filesystem::weakly_canonical(scenePath);
    AssetId id = m_assetDatabase->FindAssetIdByPath(canonical);
    if (id.IsNull())
        return;

    if (DWorld* world = m_engine->GetWorld())
    {
        world->DestroyAllWorldGameObjects();
        world->SetActiveScene(nullptr);
    }

    m_assetDatabase->ReloadAssetFromDisk(id);

    PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(id);
    if (sceneAsset)
        m_engine->LoadScene(sceneAsset->GetAssetId());
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
        std::printf("[EditorCore] ResolveObject: asset not found or failed to load (assetId=%s)\n",
            assetId.ToString().c_str());
        return nullptr;
    }

    DObject* obj = asset->FindObject(objectId);
    if (!obj)
    {
        std::printf("[EditorCore] ResolveObject: objectId not found in asset (objectId=%s assetId=%s)\n",
            objectId.ToString().c_str(), assetId.ToString().c_str());
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
        DPrimaryAsset* asset = entry.m_instance;
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

void EditorCore::EnqueueSerializedCommand(std::string jsonPayload)
{
    std::lock_guard lock(m_commandQueueMutex);
    m_pendingCommands.push_back(std::move(jsonPayload));
}

void EditorCore::DrainCommandQueue(std::vector<std::string>& outResponses)
{
    std::vector<std::string> batch;
    {
        std::lock_guard lock(m_commandQueueMutex);
        batch.swap(m_pendingCommands);
    }

    if (batch.empty())
        return;

    EditorCommandContext ctx{ *this };

    for (const auto& payload : batch)
    {
        nlohmann::json envelope;
        try
        {
            envelope = nlohmann::json::parse(payload);
        }
        catch (const nlohmann::json::parse_error& e)
        {
            DLOG(LogEditorCommand, ELogLevel::Error, "[DrainCommandQueue] JSON parse error: {}", e.what());
            outResponses.push_back(nlohmann::json{{"ok", false}, {"error", std::string{"JSON parse error: "} + e.what()}}.dump());
            continue;
        }

        std::string type = envelope.value("type", "");
        if (type.empty())
        {
            DLOG(LogEditorCommand, ELogLevel::Error, "[DrainCommandQueue] Missing 'type' field");
            outResponses.push_back(nlohmann::json{{"ok", false}, {"error", "Missing 'type' field"}}.dump());
            continue;
        }

        // MCP envelope: {"type":"command","system":"...","command":"EditorCommand_*","params":{...}}
        // Legacy envelope: {"type":"EditorCommand_*","data":{...}}
        std::string commandName;
        nlohmann::json commandData;
        if (type == "command" && envelope.contains("command"))
        {
            commandName = envelope["command"].get<std::string>();
            commandData = envelope.value("params", nlohmann::json::object());

            // Auto-inject active scene asset ID for MCP callers
            DPrimaryAsset* activeAsset = GetActiveSceneAsset();
            if (activeAsset)
            {
                std::string aid = activeAsset->GetAssetId().ToString();
                if (!commandData.contains("sceneAssetId"))
                    commandData["sceneAssetId"] = aid;
                if (!commandData.contains("assetId"))
                    commandData["assetId"] = aid;
            }
        }
        else
        {
            commandName = type;
            commandData = envelope.value("data", nlohmann::json{});
        }

        auto cmd = EditorCommandRegistry::Get().Create(commandName);
        if (!cmd)
        {
            DLOG(LogEditorCommand, ELogLevel::Error, "[DrainCommandQueue] Unknown command type: {}", commandName);
            outResponses.push_back(nlohmann::json{{"ok", false}, {"commandType", commandName}, {"error", "Unknown command type"}}.dump());
            continue;
        }

        try
        {
            if (!commandData.empty())
                cmd->Deserialize(commandData);
        }
        catch (const std::exception& e)
        {
            DLOG(LogEditorCommand, ELogLevel::Error, "[DrainCommandQueue] Deserialize failed for '{}': {}", commandName, e.what());
            outResponses.push_back(nlohmann::json{{"ok", false}, {"commandType", commandName}, {"error", std::string{"Deserialize failed: "} + e.what()}}.dump());
            continue;
        }

        std::string typeName{cmd->GetTypeName()};
        EditorCommand* cmdPtr = cmd.get();
        bool ok = m_commandManager->Execute(std::move(cmd), ctx);
        if (ok)
        {
            nlohmann::json j;
            cmdPtr->Serialize(j);
            std::string objectId = j.value("createdId", "");
            if (objectId.empty())
                objectId = j.value("createdComponentId", "");
            outResponses.push_back(
                nlohmann::json{{"ok", true}, {"commandType", typeName}, {"objectId", objectId}}.dump()
            );
        }
        else
        {
            outResponses.push_back(
                nlohmann::json{{"ok", false}, {"commandType", typeName}, {"error", "Execute() returned false"}}.dump()
            );
        }
    }

    if (!outResponses.empty())
        DLOG(LogEditorCore, ELogLevel::VeryVerbose, "Drained {} commands", outResponses.size());
}

// void EditorMain::CreateAssets()
//{
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
//     // Star mesh and material
//     {
//         DMesh* mesh = CreateDObject<DMesh>();
//         mesh->Initialize(std::wstring(L"Star.obj"));
//         std::vector<DTexture*> textures = mesh->GetTextures();
//         std::vector<DMaterial*> materials;
//         for (int i = 0; i < mesh->GetSubMeshCount(); i++) {
//             DMaterial* material = CreateDObject<DMaterial>();
//             material->Initialize(shader);
//             if (i < textures.size()) {
//                 m_assetDatabase->CreateAsset(
//                     IOManager::GetEngineImportedAssetFullPath("StarTexture_" + std::to_string(i)),
//                     PA_Texture::Create(textures[i]));
//                 material->AddTexture(textures[i]);
//             }
//             materials.push_back(material);
//
//             m_assetDatabase->CreateAsset(
//                 IOManager::GetEngineImportedAssetFullPath("StarMaterial_" + std::to_string(i)),
//                 PA_Material::Create(material));
//         }
//
//         mesh->SetMaterials(materials);
//
//         m_assetDatabase->CreateAsset(
//             IOManager::GetEngineImportedAssetFullPath("StarMesh"),
//             PA_StaticMesh::Create(mesh));
//     }
//
//     // Home mesh and material
//     {
//         DMesh* mesh = CreateDObject<DMesh>();
//         mesh->Initialize(std::wstring(L"home/source/home.fbx"));
//         std::vector<DTexture*> textures = mesh->GetTextures();
//         std::vector<DMaterial*> materials;
//         for (int i = 0; i < mesh->GetSubMeshCount(); i++) {
//             DMaterial* material = CreateDObject<DMaterial>();
//             material->Initialize(shader);
//             if (i < textures.size()) {
//                 m_assetDatabase->CreateAsset(
//                     IOManager::GetEngineImportedAssetFullPath("HomeTexture_" + std::to_string(i)),
//                     PA_Texture::Create(textures[i]));
//                 material->AddTexture(textures[i]);
//             }
//             materials.push_back(material);
//
//             m_assetDatabase->CreateAsset(
//                 IOManager::GetEngineImportedAssetFullPath("HomeMaterial_" + std::to_string(i)),
//                 PA_Material::Create(material));
//         }
//
//         mesh->SetMaterials(materials);
//
//         m_assetDatabase->CreateAsset(
//             IOManager::GetEngineImportedAssetFullPath("HomeMesh"),
//             PA_StaticMesh::Create(mesh));
//     }
// }
