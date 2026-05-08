#include <gtest/gtest.h>

#include "../EditorCoreFixture.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_RenameAsset.h"

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class EditorCommandRenameAssetFixture : public EditorCoreFixture
{
};

TEST_F(EditorCommandRenameAssetFixture, EditorCommand_RenameAsset_WithInvalidAssetId_ReturnsFalse)
{
    EditorCommandContext ctx{ *m_core };
    auto cmd = std::make_unique<EditorCommand_RenameAsset>(AssetId::Generate(), std::string("some_stem"));
    EXPECT_FALSE(m_core->GetCommandManager().Execute(std::move(cmd), ctx));
}

TEST_F(EditorCommandRenameAssetFixture, EditorCommand_RenameAsset_ToUniqueStem_UpdatePathAndUndoRestoresStem)
{
    EditorCommandContext ctx{ *m_core };
    EditorAssetDatabase* db = m_core->GetAssetDatabase();
    ASSERT_NE(db, nullptr);

    const AssetId sceneId = GetActiveSceneAssetId();
    ASSERT_FALSE(sceneId.IsNull());

    const std::filesystem::path beforePath = db->GetAssetPath(sceneId);
    ASSERT_FALSE(beforePath.empty());
    const std::string beforeStem = beforePath.stem().stem().string();

    ASSERT_TRUE(m_core->GetCommandManager().Execute(
        std::make_unique<EditorCommand_RenameAsset>(sceneId, std::string("EditorCmdStemRenamed")), ctx));

    const std::filesystem::path afterPath = db->GetAssetPath(sceneId);
    EXPECT_NE(afterPath, beforePath);

    ASSERT_TRUE(m_core->GetCommandManager().Undo(ctx));
    const std::filesystem::path restoredPath = db->GetAssetPath(sceneId);
    EXPECT_EQ(restoredPath.stem().stem().string(), beforeStem);
}
