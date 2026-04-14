#include "Editor/Mcp/McpCoreFixture.h"

#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpUndoSystemTests : public McpCoreFixture {};

TEST_F(McpUndoSystemTests, QueryStack_InitiallyEmpty)
{
    auto res = Dispatch("undo_history", "stack");
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_TRUE(res["undo_stack"].is_array());
    EXPECT_TRUE(res["undo_stack"].empty());
    EXPECT_TRUE(res["redo_stack"].is_array());
    EXPECT_FALSE(res["can_undo"].get<bool>());
    EXPECT_FALSE(res["can_redo"].get<bool>());
}

TEST_F(McpUndoSystemTests, CommandUndo_NothingToUndo_ReturnsError)
{
    auto res = Dispatch("undo_history", "Undo");
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpUndoSystemTests, CommandUndo_AfterCreate_RemovesGameObject)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());
    ASSERT_EQ(m_core->GetWorld()->GetGameObjects().size(), 1u);

    auto stackBefore = Dispatch("undo_history", "stack");
    EXPECT_TRUE(stackBefore["can_undo"].get<bool>());

    auto res = Dispatch("undo_history", "Undo");
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_EQ(res["steps_done"].get<int>(), 1);
    EXPECT_TRUE(res["can_redo"].get<bool>());

    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 0u);
}

TEST_F(McpUndoSystemTests, CommandRedo_AfterUndo_RestoresGameObject)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    Dispatch("undo_history", "Undo");
    ASSERT_EQ(m_core->GetWorld()->GetGameObjects().size(), 0u);

    auto res = Dispatch("undo_history", "Redo");
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_EQ(res["steps_done"].get<int>(), 1);

    EXPECT_EQ(m_core->GetWorld()->GetGameObjects().size(), 1u);
}

TEST_F(McpUndoSystemTests, QueryStack_AfterCommand_ShowsEntry)
{
    const std::string goId = CreateLegacyGameObject();
    ASSERT_FALSE(goId.empty());

    auto res = Dispatch("undo_history", "stack");
    EXPECT_TRUE(res["ok"].get<bool>());
    EXPECT_FALSE(res["undo_stack"].empty());
    EXPECT_TRUE(res["can_undo"].get<bool>());
}
