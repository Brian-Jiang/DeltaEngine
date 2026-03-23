#pragma once

#include "EditorIncludes.h"

#include "Runtime/Core/UUID.h"

#include <filesystem>
#include <memory>
#include <utility>

DELTA_ENGINE_NS_BEGIN

class DObject;
class EditorAssetDatabase;
class EditorSelectionState;
class EngineMain;
class DWorld;

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
    DELTAEDITOR_API EngineMain* GetEngine() { return m_engine; }
    DELTAEDITOR_API DWorld* GetWorld();

    DELTAEDITOR_API void LoadScene(const std::filesystem::path& scenePath);

    DELTAEDITOR_API DObject* FindObject(const AssetId& assetId, const ObjectId& objectId);
    DELTAEDITOR_API std::pair<AssetId, ObjectId> GetIdsForObject(DObject* obj);
    DELTAEDITOR_API void NotifyObjectDestroyed(const ObjectId& objectId);

private:
    std::unique_ptr<EditorAssetDatabase> m_assetDatabase;
    std::unique_ptr<EditorSelectionState> m_selectionState;
    EngineMain* m_engine = nullptr;
};

DELTA_ENGINE_NS_END
