#include "Editor/Mcp/McpCoreFixture.h"

#include <nlohmann/json.hpp>

#include <string>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpReflectionSystemTests : public McpCoreFixture {};

TEST_F(McpReflectionSystemTests, QueryClasses_ContainsGameObject)
{
    auto res = Dispatch("reflection", "classes");
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["classes"].is_array());

    bool foundGameObject = false;
    for (const auto& cls : res["classes"])
        if (cls["name"].get<std::string>() == "GameObject")
            foundGameObject = true;
    EXPECT_TRUE(foundGameObject);
}

TEST_F(McpReflectionSystemTests, QueryClasses_WithBaseFilter_ReturnsSubclasses)
{
    auto res = Dispatch("reflection", "classes",
                        {{"base_class_filter", "DComponent"}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["classes"].is_array());
    // At minimum PointLight, DirectionalLight, SpotLight are DComponent subclasses
    EXPECT_GE(res["classes"].size(), 1u);
}

TEST_F(McpReflectionSystemTests, QueryClassSchema_PointLight_HasProperties)
{
    auto res = Dispatch("reflection", "class_schema",
                        {{"class_name", "PointLight"}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res.contains("schema"));
    EXPECT_EQ(res["schema"]["class_name"].get<std::string>(), "PointLight");
    EXPECT_TRUE(res["schema"]["properties"].is_array());
    EXPECT_GE(res["schema"]["properties"].size(), 1u);
}

TEST_F(McpReflectionSystemTests, QueryClassSchema_UnknownClass_ReturnsError)
{
    auto res = Dispatch("reflection", "class_schema",
                        {{"class_name", "DoesNotExistClass"}});
    EXPECT_FALSE(res["ok"].get<bool>());
    EXPECT_TRUE(res.contains("error"));
}

TEST_F(McpReflectionSystemTests, QueryInheritanceChain_PointLight_ContainsAncestors)
{
    auto res = Dispatch("reflection", "inheritance_chain",
                        {{"class_name", "PointLight"}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["chain"].is_array());
    EXPECT_GE(res["chain"].size(), 2u);

    // Chain: PointLight → LightComponent → SceneComponent → ... → DObject
    bool foundDObject = false;
    for (const auto& name : res["chain"])
        if (name.get<std::string>() == "DObject")
            foundDObject = true;
    EXPECT_TRUE(foundDObject);
}

TEST_F(McpReflectionSystemTests, QueryFindClassesWithProperty_FindsMatchingClasses)
{
    auto res = Dispatch("reflection", "find_classes_with_property",
                        {{"property_name", "m_name"}});
    EXPECT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["matches"].is_array());
    EXPECT_GE(res["matches"].size(), 1u);
}
