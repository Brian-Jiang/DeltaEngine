#include "EditorCoreFixture.h"

#include "Editor/Commands/EditorAuxiliarySceneCommands.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"
#include "Editor/Commands/EditorCommand_RenameObject.h"

#include <nlohmann/json.hpp>

#include "Runtime/Core/GameObject.h"

#include <fstream>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class EditorCommandTests_Scene_SaveLoad : public EditorCoreFixture
{
};

TEST_F(EditorCommandTests_Scene_SaveLoad, EditorCommand_SaveScene_WritesSceneFileToDisk)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));

    m_core->GetCommandManager().ExecuteAuxiliary(
        std::make_unique<EditorAuxiliaryCommand_SaveScene>(), ctx);

    ASSERT_TRUE(std::filesystem::exists(m_defaultScenePath));
    std::ifstream in(m_defaultScenePath);
    ASSERT_TRUE(in.good());
    const nlohmann::json j = nlohmann::json::parse(in);
    EXPECT_TRUE(j.contains("objects"));
}

TEST_F(EditorCommandTests_Scene_SaveLoad, EditorCommand_LoadScene_ClearsUndoStack)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));
    EXPECT_TRUE(m_core->GetCommandManager().CanUndo());

    m_core->GetCommandManager().ExecuteAuxiliary(
        std::make_unique<EditorAuxiliaryCommand_LoadScene>(m_defaultScenePath), ctx);

    EXPECT_FALSE(m_core->GetCommandManager().CanUndo());
}

TEST_F(EditorCommandTests_Scene_SaveLoad, EditorCommand_LoadScene_RestoresGameObjects)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneId = GetActiveSceneAssetId();

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneId, "GameObject"), ctx));
    GameObject* go = m_core->GetWorld()->GetGameObjects().front();
    const ObjectId goId = go->GetObjectId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_RenameObject>(sceneId, goId, std::string("SaveMe")), ctx));

    m_core->GetCommandManager().ExecuteAuxiliary(
        std::make_unique<EditorAuxiliaryCommand_SaveScene>(), ctx);

    m_core->GetCommandManager().ExecuteAuxiliary(
        std::make_unique<EditorAuxiliaryCommand_LoadScene>(m_defaultScenePath), ctx);

    bool found = false;
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
    {
        if (g->GetName() == "SaveMe")
        {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}
