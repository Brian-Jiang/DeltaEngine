#include "Runtime/Assets/PA_DScene.h"

#include "Runtime/Core/DScene.h"
#include "Runtime/Test/SerializationTestTypes.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(PA_DSceneTests, Create_EmptySceneName_SetsSceneNameAndNonNullScene)
{
    PA_DScene* asset = PA_DScene::Create("");
    ASSERT_NE(asset, nullptr);

    DScene* scene = asset->GetScene();
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene->GetName(), "");
    EXPECT_EQ(scene->GetOwningAsset(), asset);
}

TEST(PA_DSceneTests, Create_WithName_ReturnsMatchingSceneObject)
{
    PA_DScene* asset = PA_DScene::Create("LevelOne");
    ASSERT_NE(asset, nullptr);

    DScene* scene = asset->GetScene();
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene->GetName(), "LevelOne");
}

TEST(PA_DSceneTests, GetScene_WithMultipleOwnedObjects_ReturnsFirstDSceneInstance)
{
    PA_DScene* asset = PA_DScene::Create("PrimaryScene");
    ASSERT_NE(asset, nullptr);

    DTestObjectA* extra = CreateDObject<DTestObjectA>();
    ASSERT_NE(extra, nullptr);
    asset->AddObject(extra);

    DScene* scene = asset->GetScene();
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene->GetName(), "PrimaryScene");
}
