#include <gtest/gtest.h>

#include "Editor/Commands/EditorCommandRegistry.h"
#include "Editor/Commands/EditorCommand.h"
#include "Editor/Commands/EditorCommand_SetTestValue.h"

#include <algorithm>
#include <string>

using namespace DeltaEngine;

class EditorCommandRegistryFixture : public ::testing::Test
{
};

TEST_F(EditorCommandRegistryFixture, EditorCommandRegistry_Create_UnknownType_ReturnsNull)
{
    const std::unique_ptr<EditorCommand> cmd =
        EditorCommandRegistry::Get().Create("EditorCommand_TypeThatDoesNotExist");
    EXPECT_FALSE(static_cast<bool>(cmd));
}

TEST_F(EditorCommandRegistryFixture, EditorCommandRegistry_GetCommandNames_ContainsSetTestValue)
{
    const auto names = EditorCommandRegistry::Get().GetCommandNames();
    const auto it = std::find(names.begin(), names.end(), std::string{EditorCommand_SetTestValue::StaticTypeName()});
    EXPECT_NE(it, names.end());
}
