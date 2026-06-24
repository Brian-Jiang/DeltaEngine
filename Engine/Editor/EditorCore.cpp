#include "EditorCore.h"

#include "Editor/Animation/EditorAnimationManager.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/EditorCommand.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommandRegistry.h"
#include "Editor/Commands/EditorAuxiliarySceneCommands.h"
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
        const std::filesystem::path root = std::filesystem::weakly_canonical(assetRootOverride);
        PA_DScene* defaultScene = PA_DScene::Create("DefaultScene");
        m_assetDatabase->CreateAsset(root / "DefaultScene.dasset.json", defaultScene);

        m_assetDatabase->ScanAssetsFolder(root);

        PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(
            m_assetDatabase->FindAssetIdByPath(root / "DefaultScene.dasset.json"));
        if (sceneAsset)
            m_engine->LoadScene(sceneAsset->GetAssetId());
        else
            DLOG(LogEditorCore, ELogLevel::Error,
                 "Failed to load DefaultScene (assetRootOverride='{}'); expected DefaultScene.dasset.json under that root",
                 assetRootOverride.string());
    }
    else
    {
        m_assetDatabase->ScanAssetsFolder(IOManager::GetEngineImportedAssetsFolder());

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

    if (!m_headless)
    {
        constexpr uint16_t kPreferredMcpPort = 57340;
        auto router = std::make_shared<McpQueryRouter>(*this, *m_mcpRegistry);
        g_mcpServer = std::make_unique<McpSocketServer>(
            [router](const std::string& json) -> std::string { return router->Route(json); },
            [router](const std::string& json) -> std::string { return router->Route(json); });
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
    g_mcpServer.reset();
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

nlohmann::json EditorCore::ReimportAssets(const std::vector<AssetId>& assetIds)
{
    nlohmann::json reimported = nlohmann::json::array();
    nlohmann::json skipped = nlohmann::json::array();

    for (const AssetId& id : assetIds)
    {
        DPrimaryAsset* asset = m_assetDatabase ? m_assetDatabase->LoadAsset(id) : nullptr;
        if (!asset)
        {
            skipped.push_back({{"asset_id", id.ToString()}, {"reason", "not found or failed to load"}});
            continue;
        }

        if (auto* shaderAsset = dynamic_cast<PA_Shader*>(asset))
        {
            DShader* shader = shaderAsset->GetShader();
            if (!shader)
            {
                skipped.push_back({{"asset_id", id.ToString()}, {"reason", "shader asset has no shader"}});
                continue;
            }

            DLOG(LogEditorCore, ELogLevel::Verbose, "ReimportAssets: reimporting shader '{}'", id.ToString());
            shader->Reimport();
            reimported.push_back(id.ToString());
            continue;
        }

        skipped.push_back({{"asset_id", id.ToString()}, {"reason", "not a shader asset"}});
    }

    return { {"ok", true}, {"reimported", reimported}, {"skipped", skipped} };
}

void EditorCore::SetActiveMcpRequestId(std::string requestId)
{
    m_activeMcpRequestId = std::move(requestId);
}

void EditorCore::ClearActiveMcpRequestId()
{
    m_activeMcpRequestId.clear();
}

void EditorCore::EnqueueSerializedCommand(std::string jsonPayload)
{
    if (!DELTA_ENSURE(!jsonPayload.empty()))
    {
        DLOG(LogEditorCore, ELogLevel::Warning,
             "EnqueueSerializedCommand: empty payload dropped (expected non-empty JSON envelope)");
        return;
    }

    if (!m_activeMcpRequestId.empty())
    {
        nlohmann::json envelope;
        try
        {
            envelope = nlohmann::json::parse(jsonPayload);
        }
        catch (const nlohmann::json::parse_error& e)
        {
            DLOG(LogEditorCore, ELogLevel::Warning,
                 "EnqueueSerializedCommand: JSON parse error: {}", e.what());
            return;
        }

        envelope["request_id"] = m_activeMcpRequestId;
        jsonPayload = envelope.dump();
    }

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

    DLOG(LogEditorCore, ELogLevel::Verbose, "DrainCommandQueue: pending={}", batch.size());

    EditorCommandContext ctx{ *this };

    auto emitResponse = [&](const nlohmann::json& envelope, nlohmann::json payload) {
        const std::string requestId = envelope.value("request_id", "");
        outResponses.push_back(MakeResultResponse(requestId, std::move(payload)).dump());
    };

    auto emitResponseNoEnvelope = [&](nlohmann::json payload) {
        outResponses.push_back(MakeResultResponse("", std::move(payload)).dump());
    };

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
            emitResponseNoEnvelope({{"ok", false}, {"error", std::string{"JSON parse error: "} + e.what()}});
            continue;
        }

        std::string type = envelope.value("type", "");
        if (type.empty())
        {
            DLOG(LogEditorCommand, ELogLevel::Error, "[DrainCommandQueue] Missing 'type' field");
            emitResponse(envelope, {{"ok", false}, {"error", "Missing 'type' field"}});
            continue;
        }

        if (type == "auxiliary")
        {
            std::string name = envelope.value("name", "");
            if (name == "SaveDirtyAssets")
            {
                m_commandManager->ExecuteAuxiliary(
                    std::make_unique<EditorAuxiliaryCommand_SaveScene>(), ctx);
                emitResponse(envelope, {{"ok", true}, {"commandType", "SaveDirtyAssets"}});
                continue;
            }
            if (name == "LoadScene")
            {
                std::string scenePath = envelope.value("scenePath", "");
                if (scenePath.empty())
                {
                    emitResponse(envelope,
                        {{"ok", false}, {"commandType", "LoadScene"},
                         {"error", "Missing 'scenePath' field"}});
                    continue;
                }
                m_commandManager->ExecuteAuxiliary(
                    std::make_unique<EditorAuxiliaryCommand_LoadScene>(
                        std::filesystem::path(scenePath)), ctx);
                emitResponse(envelope, {{"ok", true}, {"commandType", "LoadScene"}});
                continue;
            }
            if (name == "StartLightAnimation")
            {
                const AssetId  animAssetId  = UUID::FromString(envelope.value("assetId", ""));
                const ObjectId animObjectId = UUID::FromString(envelope.value("objectId", ""));
                const std::string propName  = envelope.value("propertyName", "m_intensity");
                const float targetValue     = envelope.value("targetValue", 0.0f);
                const float duration        = envelope.value("duration", 0.0f);

                LightComponent* light = ResolveObject<LightComponent>(animAssetId, animObjectId);
                if (!light)
                {
                    emitResponse(envelope,
                        {{"ok", false}, {"commandType", "StartLightAnimation"},
                         {"error", "object not found or not a LightComponent"}});
                    continue;
                }

                const float current = light->GetIntensity();
                if (m_animationManager)
                {
                    m_animationManager->StartAnimation(
                        animAssetId, animObjectId, propName,
                        current, targetValue, duration,
                        [light](float v) { light->SetIntensity(v); });
                }
                else
                {
                    // Headless fallback: apply immediately via SetProperty.
                    auto cmd = std::make_unique<EditorCommand_SetProperty>(
                        animAssetId, animObjectId, propName,
                        nlohmann::json(current),
                        nlohmann::json(targetValue));
                    m_commandManager->Execute(std::move(cmd), ctx);
                }
                emitResponse(envelope, {{"ok", true}, {"commandType", "StartLightAnimation"}});
                continue;
            }
            if (name == "StartTransformChannelAnimation")
            {
                using namespace DirectX;
                using namespace DirectX::SimpleMath;

                const AssetId  scAssetId  = UUID::FromString(envelope.value("scAssetId",  ""));
                const ObjectId scObjectId = UUID::FromString(envelope.value("scObjectId", ""));
                const std::string channel = envelope.value("channel", "");
                const std::string space   = envelope.value("space",   "local");
                const float duration      = envelope.value("duration", 1.0f);
                const bool worldSpace     = (space == "world");

                if (scAssetId.IsNull() || scObjectId.IsNull() || channel.empty())
                {
                    emitResponse(envelope,
                        {{"ok", false}, {"commandType", "StartTransformChannelAnimation"},
                         {"error", "Missing scAssetId, scObjectId, or channel"}});
                    continue;
                }

                SceneComponent* sc = ResolveObject<SceneComponent>(scAssetId, scObjectId);
                if (!sc)
                {
                    emitResponse(envelope,
                        {{"ok", false}, {"commandType", "StartTransformChannelAnimation"},
                         {"error", "SceneComponent not found"}});
                    continue;
                }

                DProperty* transformProp = sc->GetClass()->FindPropertyByName("m_localTransform");
                if (!transformProp)
                {
                    emitResponse(envelope,
                        {{"ok", false}, {"commandType", "StartTransformChannelAnimation"},
                         {"error", "m_localTransform property not found"}});
                    continue;
                }

                const nlohmann::json snapshot = PropertyToJson(sc, transformProp);
                const auto& tval = envelope["targetValue"];

                if (channel == "position")
                {
                    const Vector3 from   = worldSpace ? sc->GetWorldPosition() : sc->GetLocalPosition();
                    const Vector3 target(tval[0].get<float>(), tval[1].get<float>(), tval[2].get<float>());

                    if (m_animationManager)
                    {
                        m_animationManager->StartAnimationVec3(
                            scAssetId, scObjectId, "position", from, target, duration,
                            worldSpace
                                ? std::function<void(Vector3)>([sc](Vector3 p) { sc->SetWorldPosition(p); })
                                : std::function<void(Vector3)>([sc](Vector3 p) { sc->SetLocalPosition(p); }),
                            snapshot);
                    }
                    else
                    {
                        if (worldSpace) sc->SetWorldPosition(target); else sc->SetLocalPosition(target);
                        auto cmd = std::make_unique<EditorCommand_SetProperty>(
                            scAssetId, scObjectId, "m_localTransform",
                            snapshot, PropertyToJson(sc, transformProp));
                        m_commandManager->Execute(std::move(cmd), ctx);
                    }
                }
                else if (channel == "rotation")
                {
                    const Quaternion from = worldSpace ? sc->GetWorldRotation() : sc->GetLocalRotation();
                    Quaternion target;
                    if (tval.size() == 4)
                        target = Quaternion(tval[0].get<float>(), tval[1].get<float>(),
                                            tval[2].get<float>(), tval[3].get<float>());
                    else
                        target = Quaternion::CreateFromYawPitchRoll(
                            tval[1].get<float>(), tval[0].get<float>(), tval[2].get<float>());

                    if (m_animationManager)
                    {
                        m_animationManager->StartAnimationQuat(
                            scAssetId, scObjectId, "rotation", from, target, duration,
                            worldSpace
                                ? std::function<void(Quaternion)>([sc](Quaternion q) { sc->SetWorldRotation(q); })
                                : std::function<void(Quaternion)>([sc](Quaternion q) { sc->SetLocalRotation(q); }),
                            snapshot);
                    }
                    else
                    {
                        if (worldSpace) sc->SetWorldRotation(target); else sc->SetLocalRotation(target);
                        auto cmd = std::make_unique<EditorCommand_SetProperty>(
                            scAssetId, scObjectId, "m_localTransform",
                            snapshot, PropertyToJson(sc, transformProp));
                        m_commandManager->Execute(std::move(cmd), ctx);
                    }
                }
                else if (channel == "scale")
                {
                    const Vector3 from   = sc->GetLocalScale();
                    const Vector3 target(tval[0].get<float>(), tval[1].get<float>(), tval[2].get<float>());

                    if (m_animationManager)
                    {
                        m_animationManager->StartAnimationVec3(
                            scAssetId, scObjectId, "scale", from, target, duration,
                            [sc](Vector3 s) { sc->SetLocalScale(s); },
                            snapshot);
                    }
                    else
                    {
                        sc->SetLocalScale(target);
                        auto cmd = std::make_unique<EditorCommand_SetProperty>(
                            scAssetId, scObjectId, "m_localTransform",
                            snapshot, PropertyToJson(sc, transformProp));
                        m_commandManager->Execute(std::move(cmd), ctx);
                    }
                }
                else
                {
                    emitResponse(envelope,
                        {{"ok", false}, {"commandType", "StartTransformChannelAnimation"},
                         {"error", "Unknown channel: " + channel}});
                    continue;
                }

                emitResponse(envelope, {{"ok", true}, {"commandType", "StartTransformChannelAnimation"}});
                continue;
            }
            if (name == "ReimportAssets")
            {
                std::vector<AssetId> ids;
                if (envelope.contains("assetIds") && envelope["assetIds"].is_array())
                {
                    for (const auto& v : envelope["assetIds"])
                    {
                        if (!v.is_string())
                            continue;
                        const AssetId id = UUID::FromString(v.get<std::string>());
                        if (!id.IsNull())
                            ids.push_back(id);
                    }
                }

                nlohmann::json result = ReimportAssets(ids);
                result["commandType"] = "ReimportAssets";
                emitResponse(envelope, std::move(result));
                continue;
            }
            emitResponse(envelope, {{"ok", false}, {"error", "Unknown auxiliary: " + name}});
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
            emitResponse(envelope, {{"ok", false}, {"commandType", commandName}, {"error", "Unknown command type"}});
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
            emitResponse(envelope,
                {{"ok", false}, {"commandType", commandName},
                 {"error", std::string{"Deserialize failed: "} + e.what()}});
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
            emitResponse(envelope,
                {{"ok", true}, {"commandType", typeName}, {"objectId", objectId}});
        }
        else
        {
            emitResponse(envelope,
                {{"ok", false}, {"commandType", typeName}, {"error", "Execute() returned false"}});
        }
    }

}

 void EditorCore::CreateAssets()
 {
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
     //    psShader->Initialize(L"Shaders/PostProcess_Passthrough.hlsl", L"VSMain", L"PSMain", L"vs_6_0", L"ps_6_0");
     //    PA_Shader *psShaderPA = PA_Shader::Create(psShader);
     //    m_assetDatabase->CreateAsset(psShaderPath, psShaderPA);
     //}

     //{
     //    const std::filesystem::path psShaderPath = IOManager::GetEngineImportedAssetFullPath("TonemapShader");
     //    DShader *psShader = CreateDObject<DShader>();
     //    psShader->Initialize(L"Shaders/PostProcess_Tonemap.hlsl", L"VSMain", L"PSMain", L"vs_6_0", L"ps_6_0");
     //    PA_Shader *psShaderPA = PA_Shader::Create(psShader);
     //    m_assetDatabase->CreateAsset(psShaderPath, psShaderPA);
     //}

     //const std::filesystem::path ppStackPath = IOManager::GetEngineImportedAssetFullPath("PostProcessStack");
     //PA_PostProcessStack *ppStack = PA_PostProcessStack::Create();
     //ppStack->AddPass("PassthroughPass");
     //ppStack->AddPass("TonemapPass");
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
