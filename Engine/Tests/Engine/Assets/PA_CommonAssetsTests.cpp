#include "Runtime/Assets/PA_CommonAssets.h"

#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/Core/DShader.h"
#include "Runtime/Core/DTexture.h"
#include "Runtime/Core/Skybox.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;

TEST(PA_CommonAssetsTests, PA_Shader_CreateWithPayload_GetShader_ReturnsSameInstance)
{
    DShader* shader = CreateDObject<DShader>();
    ASSERT_NE(shader, nullptr);

    PA_Shader* asset = PA_Shader::Create(shader);
    ASSERT_NE(asset, nullptr);
    EXPECT_FALSE(asset->GetAssetId().IsNull());
    EXPECT_EQ(asset->GetHeader().m_className, "PA_Shader");
    EXPECT_EQ(asset->GetShader(), shader);
}

TEST(PA_CommonAssetsTests, PA_Shader_CreateWithoutPayload_GetShader_ReturnsNull)
{
    PA_Shader* asset = PA_Shader::Create(nullptr);
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetShader(), nullptr);
}

TEST(PA_CommonAssetsTests, PA_Material_CreateWithPayload_GetMaterial_ReturnsSameInstance)
{
    DMaterial* material = CreateDObject<DMaterial>();
    ASSERT_NE(material, nullptr);

    PA_Material* asset = PA_Material::Create(material);
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetMaterial(), material);
}

TEST(PA_CommonAssetsTests, PA_Texture_CreateWithPayload_GetTexture_ReturnsSameInstance)
{
    DTexture* texture = CreateDObject<DTexture>();
    ASSERT_NE(texture, nullptr);

    PA_Texture* asset = PA_Texture::Create(texture);
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetTexture(), texture);
}

TEST(PA_CommonAssetsTests, PA_StaticMesh_CreateWithPayload_GetStaticMesh_ReturnsSameInstance)
{
    DMesh* mesh = CreateDObject<DMesh>();
    ASSERT_NE(mesh, nullptr);

    PA_StaticMesh* asset = PA_StaticMesh::Create(mesh);
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetStaticMesh(), mesh);
}

TEST(PA_CommonAssetsTests, PA_Skybox_CreateWithPayload_GetSkybox_ReturnsSameInstance)
{
    Skybox* skybox = CreateDObject<Skybox>();
    ASSERT_NE(skybox, nullptr);

    PA_Skybox* asset = PA_Skybox::Create(skybox);
    ASSERT_NE(asset, nullptr);
    EXPECT_EQ(asset->GetSkybox(), skybox);
}
