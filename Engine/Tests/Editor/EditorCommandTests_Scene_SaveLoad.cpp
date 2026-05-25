#include "EditorCoreFixture.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/EditorAuxiliarySceneCommands.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"
#include "Editor/Commands/EditorCommand_RenameObject.h"

#include <nlohmann/json.hpp>

#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/DScene.h"
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

TEST_F(EditorCommandTests_Scene_SaveLoad, LoadScene_SwapsActiveSceneAndTearsDownGameObjects)
{
    EditorCommandContext ctx{ *m_core };

    // Scene A is the fixture's DefaultScene. Add a GameObject so we can verify teardown.
    const AssetId sceneAId = GetActiveSceneAssetId();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneAId, "GameObject"), ctx));
    GameObject* goA = m_core->GetWorld()->GetGameObjects().front();
    ASSERT_NE(goA, nullptr);
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_RenameObject>(sceneAId, goA->GetObjectId(),
                                                     std::string("OnlyInSceneA")), ctx));
    m_core->GetCommandManager().ExecuteAuxiliary(
        std::make_unique<EditorAuxiliaryCommand_SaveScene>(), ctx);

    // Create scene B on disk via the asset database.
    const std::filesystem::path sceneBPath =
        std::filesystem::weakly_canonical(m_tempDir / "SecondScene.dasset.json");
    PA_DScene* sceneB = PA_DScene::Create("SecondScene");
    m_core->GetAssetDatabase()->CreateAsset(sceneBPath, sceneB);
    m_core->GetAssetDatabase()->SaveDirtyAssets();
    ASSERT_TRUE(std::filesystem::exists(sceneBPath));

    // Swap to scene B.
    m_core->GetCommandManager().ExecuteAuxiliary(
        std::make_unique<EditorAuxiliaryCommand_LoadScene>(sceneBPath), ctx);

    // Active scene is now B.
    DPrimaryAsset* active = m_core->GetActiveSceneAsset();
    ASSERT_NE(active, nullptr);
    EXPECT_NE(active->GetAssetId(), sceneAId);

    // Scene A's GameObject must be gone (teardown happened before scene B loaded).
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
        EXPECT_NE(g->GetName(), std::string("OnlyInSceneA"));
}

TEST_F(EditorCommandTests_Scene_SaveLoad, LoadScene_SwitchThenSave_PreservesPreviousSceneObjects)
{
    EditorCommandContext ctx{ *m_core };
    const AssetId sceneAId = GetActiveSceneAssetId();

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_CreateGameObject>(sceneAId, "GameObject"), ctx));
    GameObject* goA = m_core->GetWorld()->GetGameObjects().front();
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_RenameObject>(sceneAId, goA->GetObjectId(),
                                                     std::string("MarkerInA")), ctx));
    m_core->GetCommandManager().ExecuteAuxiliary(
        std::make_unique<EditorAuxiliaryCommand_SaveScene>(), ctx);

    const std::filesystem::path sceneBPath =
        std::filesystem::weakly_canonical(m_tempDir / "SecondScene.dasset.json");
    PA_DScene* sceneB = PA_DScene::Create("SecondScene");
    m_core->GetAssetDatabase()->CreateAsset(sceneBPath, sceneB);
    m_core->GetAssetDatabase()->SaveDirtyAssets();

    m_core->GetCommandManager().ExecuteAuxiliary(
        std::make_unique<EditorAuxiliaryCommand_LoadScene>(sceneBPath), ctx);
    m_core->GetCommandManager().ExecuteAuxiliary(
        std::make_unique<EditorAuxiliaryCommand_SaveScene>(), ctx);

    m_core->GetAssetDatabase()->ReloadAssetFromDisk(sceneAId);
    PA_DScene* reloadedA = m_core->GetAssetDatabase()->LoadAsset<PA_DScene>(sceneAId);
    ASSERT_NE(reloadedA, nullptr);
    DScene* scene = reloadedA->GetScene();
    ASSERT_NE(scene, nullptr);

    bool found = false;
    for (GameObject* g : scene->GetGameObjects())
    {
        if (g->GetName() == "MarkerInA")
            found = true;
    }
    EXPECT_TRUE(found);
}
