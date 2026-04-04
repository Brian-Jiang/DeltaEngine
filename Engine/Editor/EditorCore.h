#pragma once

#include "EditorIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/UUID.h"

#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

DELTA_ENGINE_NS_BEGIN

class DComponent;
class DPrimaryAsset;
class EditorAssetDatabase;
class EditorSelectionState;
class EngineMain;
class DWorld;
class GameObject;

class EditorCommandManager;
class EditorCore;
extern EditorCore* g_editorCore;

class EditorCore
{
public:
    DELTAEDITOR_API EditorCore();
    DELTAEDITOR_API ~EditorCore();

    DELTAEDITOR_API void Initialize(EngineMain& engine);
    DELTAEDITOR_API void Shutdown();

    DELTAEDITOR_API EditorAssetDatabase* GetAssetDatabase() { return m_assetDatabase.get(); }
    DELTAEDITOR_API EditorSelectionState* GetSelectionState() { return m_selectionState.get(); }
    DELTAEDITOR_API EditorCommandManager& GetCommandManager() { return *m_commandManager; }
    DELTAEDITOR_API EngineMain* GetEngine() { return m_engine; }
    DELTAEDITOR_API DWorld* GetWorld();
    DELTAEDITOR_API DPrimaryAsset* GetActiveSceneAsset();

    DELTAEDITOR_API void LoadScene(const std::filesystem::path& scenePath);

    DELTAEDITOR_API ObjectId CreateGameObject(std::string_view name, GameObject** outPtr = nullptr);
    DELTAEDITOR_API ObjectId AddComponentToGameObject(ObjectId gameObjectId, std::string_view componentClassName);

    // todo remove test value when editor command system is in place to support arbitrary data in commands
    DELTAEDITOR_API void SetTestValue(const std::string& key, const std::string& value);
    DELTAEDITOR_API std::string GetTestValue(const std::string& key) const;

    DELTAEDITOR_API DObject* ResolveObject(const AssetId& assetId, const ObjectId& objectId);

    template<typename T>
    T* ResolveObject(const AssetId& assetId, const ObjectId& objectId)
    {
        return dynamic_cast<T*>(ResolveObject(assetId, objectId));
    }

    DELTAEDITOR_API std::pair<AssetId, ObjectId> GetIdsForObject(DObject* obj);
    DELTAEDITOR_API void NotifyObjectDestroyed(const ObjectId& objectId);

    // Serialized command dispatch queue for headless / MCP callers.
    // JSON envelope: { "type": "EditorCommand_SetProperty", "data": { ... } }
    DELTAEDITOR_API void EnqueueSerializedCommand(std::string jsonPayload);
    DELTAEDITOR_API void DrainCommandQueue();

private:
    std::unique_ptr<EditorAssetDatabase> m_assetDatabase;
    std::unique_ptr<EditorSelectionState> m_selectionState;
    std::unique_ptr<EditorCommandManager> m_commandManager;
    std::unordered_map<std::string, std::string> m_testValueStore;
    EngineMain* m_engine = nullptr;

    std::mutex m_commandQueueMutex;
    std::vector<std::string> m_pendingCommands;
};

DELTA_ENGINE_NS_END
