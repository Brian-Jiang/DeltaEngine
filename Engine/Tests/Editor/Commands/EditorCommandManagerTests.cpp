#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "../EditorCoreFixture.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_SetTestValue.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class EditorCommandManagerFixture : public EditorCoreFixture
{
};

TEST_F(EditorCommandManagerFixture, EditorCommandManager_DeserializeAndReplay_RestoresUndoStackAndValues)
{
    EditorCommandContext ctx{ *m_core };
    auto& mgr = m_core->GetCommandManager();

    ASSERT_TRUE(mgr.Execute(std::make_unique<EditorCommand_SetTestValue>("replayKey", "", "after"), ctx));
    EXPECT_EQ(m_core->GetTestValue("replayKey"), "after");

    nlohmann::json stored;
    mgr.SerializeUndoStack(stored);
    mgr.Clear();
    EXPECT_EQ(mgr.GetUndoStackDepth(), 0u);

    mgr.DeserializeAndReplay(stored, ctx);
    EXPECT_EQ(mgr.GetUndoStackDepth(), 1u);
    EXPECT_EQ(m_core->GetTestValue("replayKey"), "after");
}

TEST_F(EditorCommandManagerFixture, EditorCommandManager_DeserializeAndReplay_SkipsUnknownCommandType)
{
    EditorCommandContext ctx{ *m_core };
    auto& mgr = m_core->GetCommandManager();

    nlohmann::json stored;

    nlohmann::json unknownEntry;
    unknownEntry["type"] = "EditorCommand_NotRegistered";
    unknownEntry["data"] = nlohmann::json::object();

    nlohmann::json goodEntry;
    goodEntry["type"] = std::string{ EditorCommand_SetTestValue::StaticTypeName() };
    goodEntry["data"] =
        { { "key", "k2" }, { "valueBefore", "" }, { "valueAfter", "ok" } };

    stored["undoStack"] = nlohmann::json::array({ unknownEntry, goodEntry });

    mgr.DeserializeAndReplay(stored, ctx);
    EXPECT_EQ(m_core->GetTestValue("k2"), "ok");
    EXPECT_EQ(mgr.GetUndoStackDepth(), 1u);
}

TEST_F(EditorCommandManagerFixture, EditorCommandManager_DeserializeAndReplay_MissingUndoStackKey_DoesNothing)
{
    EditorCommandContext ctx{ *m_core };
    auto& mgr = m_core->GetCommandManager();
    const size_t before = mgr.GetUndoStackDepth();

    mgr.DeserializeAndReplay(nlohmann::json::object(), ctx);
    EXPECT_EQ(mgr.GetUndoStackDepth(), before);
}

TEST_F(EditorCommandManagerFixture, EditorCommandManager_DeserializeAndReplay_MissingDataObject_SkipsEntry)
{
    EditorCommandContext ctx{ *m_core };
    auto& mgr = m_core->GetCommandManager();

    nlohmann::json stored;
    stored["undoStack"] = nlohmann::json::array({
        {{"type", std::string{EditorCommand_SetTestValue::StaticTypeName()}}}});

    mgr.DeserializeAndReplay(stored, ctx);
    EXPECT_EQ(mgr.GetUndoStackDepth(), 0u);
}
