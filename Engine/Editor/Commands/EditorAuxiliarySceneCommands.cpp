#include "Editor/Commands/EditorAuxiliarySceneCommands.h"
#include "Editor/Commands/EditorCommand.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"

using namespace DeltaEngine;

void EditorAuxiliaryCommand_SaveScene::Execute(EditorCommandContext& ctx)
{
    if (EditorAssetDatabase* db = ctx.core.GetAssetDatabase())
    {
        db->SaveDirtyAssets();
        DLOG(LogEditorCommand, ELogLevel::Log, "[SaveScene] Scene saved");
    }
}

EditorAuxiliaryCommand_LoadScene::EditorAuxiliaryCommand_LoadScene(std::filesystem::path scenePath)
    : m_scenePath(std::move(scenePath))
{
}

void EditorAuxiliaryCommand_LoadScene::Execute(EditorCommandContext& ctx)
{
    ctx.core.LoadScene(m_scenePath);
    ctx.core.GetCommandManager().Clear();
    DLOG(LogEditorCommand, ELogLevel::Log, "[LoadScene] Scene loaded, undo stack cleared");
}
