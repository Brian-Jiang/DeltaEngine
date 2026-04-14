#pragma once

#include "EditorIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Core/UUID.h"

#include <filesystem>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

DELTA_ENGINE_NS_BEGIN

DECLARE_LOG_CATEGORY(LogEditorCore)

class DPrimaryAsset;
class EditorAssetDatabase;
class EditorSelectionState;
class EngineMain;
class DWorld;

class EditorCommandManager;
class EditorCore;
class McpRegistry;
class McpSocketServer;

extern EditorCore* g_editorCore;
extern std::unique_ptr<McpSocketServer> g_mcpServer;

class EditorCore
{
public:
    DELTAEDITOR_API EditorCore();
    DELTAEDITOR_API ~EditorCore();

    DELTAEDITOR_API void Initialize(EngineMain& engine, bool headless = false,
                                    std::filesystem::path assetRootOverride = {});
    DELTAEDITOR_API void Shutdown();

    DELTAEDITOR_API EditorAssetDatabase* GetAssetDatabase() { return m_assetDatabase.get(); }
    DELTAEDITOR_API EditorSelectionState* GetSelectionState() { return m_selectionState.get(); }
    DELTAEDITOR_API EditorCommandManager& GetCommandManager() { return *m_commandManager; }
    DELTAEDITOR_API EngineMain* GetEngine() { return m_engine; }
    DELTAEDITOR_API DWorld* GetWorld();
    DELTAEDITOR_API DPrimaryAsset* GetActiveSceneAsset();

    DELTAEDITOR_API void LoadScene(const std::filesystem::path& scenePath);

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
    DELTAEDITOR_API void DrainCommandQueue(std::vector<std::string>& outResponses);

    DELTAEDITOR_API McpRegistry* GetMcpRegistry() { return m_mcpRegistry.get(); }

private:
    std::unique_ptr<EditorAssetDatabase> m_assetDatabase;
    std::unique_ptr<EditorSelectionState> m_selectionState;
    std::unique_ptr<EditorCommandManager> m_commandManager;
    std::unordered_map<std::string, std::string> m_testValueStore;
    EngineMain* m_engine = nullptr;

    std::mutex m_commandQueueMutex;
    std::vector<std::string> m_pendingCommands;
    std::unique_ptr<McpRegistry> m_mcpRegistry;
    bool m_headless = false;
};

DELTA_ENGINE_NS_END
