#include "EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_SetTestValue.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class EditorCommandTests_UndoStack : public EditorCoreFixture
{
};

TEST_F(EditorCommandTests_UndoStack, EditorCommand_UndoStack_CapAt100)
{
    EditorCommandContext ctx{ *m_core };
    for (int i = 0; i < 101; ++i)
    {
        const std::string key = "k" + std::to_string(i);
        ASSERT_TRUE(m_core->GetCommandManager().Execute(
            std::make_unique<EditorCommand_SetTestValue>(key, "", "v"), ctx));
    }
    EXPECT_EQ(m_core->GetCommandManager().GetUndoStackDepth(), 100u);
}

TEST_F(EditorCommandTests_UndoStack, EditorCommand_Execute_ClearsRedoStack)
{
    EditorCommandContext ctx{ *m_core };
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_SetTestValue>("t", "", "A"), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_SetTestValue>("t", "A", "B"), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Redo(ctx));
    EXPECT_TRUE(m_core->GetCommandManager().CanRedo());

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_SetTestValue>("t", "A", "C"), ctx));

    EXPECT_FALSE(m_core->GetCommandManager().CanRedo());
    EXPECT_EQ(m_core->GetTestValue("t"), "C");
    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    EXPECT_EQ(m_core->GetTestValue("t"), "A");
}

TEST_F(EditorCommandTests_UndoStack, EditorCommand_Shutdown_Idempotent)
{
    EditorCommandContext ctx{ *m_core };
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_SetTestValue>("a", "", "1"), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_SetTestValue>("b", "", "2"), ctx));
    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_SetTestValue>("c", "", "3"), ctx));

    m_core->Shutdown();
    m_core->Shutdown();
    SUCCEED();
}
