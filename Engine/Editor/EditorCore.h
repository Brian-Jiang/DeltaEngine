#pragma once

#include "EditorIncludes.h"

#include <filesystem>
#include <memory>

DELTA_ENGINE_NS_BEGIN

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

private:
    std::unique_ptr<EditorAssetDatabase> m_assetDatabase;
    std::unique_ptr<EditorSelectionState> m_selectionState;
    EngineMain* m_engine = nullptr;
};

DELTA_ENGINE_NS_END
