#include "EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_CreateGameObject.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"

#include "Runtime/Core/GameObject.h"

#include <nlohmann/json.hpp>

#include <memory>
#include <set>
#include <string>
#include <vector>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class EditorCommandTests_CommandQueue : public EditorCoreFixture
{
protected:
    // Executes a SetProperty directly through the command manager.
    bool SetProperty(const AssetId& assetId, const std::string& objectId,
                     const std::string& propertyName, nlohmann::json valueAfter) const
    {
        EditorCommandContext ctx{ *m_core };
        return m_core->GetCommandManager().Execute(
            std::make_unique<EditorCommand_SetProperty>(
                assetId, DeltaEngine::UUID::FromString(objectId), propertyName,
                nlohmann::json{}, std::move(valueAfter)),
            ctx);
    }
};

TEST_F(EditorCommandTests_CommandQueue, SetProperty_AppliesImmediately)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    const std::string goId = ExecCreateGameObject();
    ASSERT_FALSE(goId.empty());

    GameObject* go = nullptr;
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
    {
        if (g->GetObjectId().ToString() == goId)
        {
            go = g;
            break;
        }
    }
    ASSERT_NE(go, nullptr);

    EXPECT_TRUE(SetProperty(sceneId, goId, "m_name", "FromQueue"));
    EXPECT_EQ(go->GetName(), "FromQueue");
}

TEST_F(EditorCommandTests_CommandQueue, InvalidObjectId_SkipsGracefully)
{
    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_FALSE(ExecCreateGameObject().empty());
    const size_t depthBefore = m_core->GetCommandManager().GetUndoStackDepth();

    SetProperty(sceneId, "11111111-1111-1111-1111-111111111111", "m_name", "X");

    EXPECT_EQ(m_core->GetCommandManager().GetUndoStackDepth(), depthBefore);
}

TEST_F(EditorCommandTests_CommandQueue, ManyCreates_ProduceDistinctObjects)
{
    for (int i = 0; i < 100; ++i)
        ASSERT_FALSE(ExecCreateGameObject().empty());

    std::set<ObjectId> ids;
    for (GameObject* g : m_core->GetWorld()->GetGameObjects())
        ids.insert(g->GetObjectId());

    EXPECT_EQ(ids.size(), 100u);
    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 100);
}

TEST_F(EditorCommandTests_CommandQueue, CreateGameObject_ReturnsObjectId)
{
    EXPECT_FALSE(ExecCreateGameObject().empty());
}
