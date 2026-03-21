#include "Runtime/EngineMain.h"
#include "Runtime/Core/Camera.h"
#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/Core/DShader.h"
#include "Runtime/Core/DTexture.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include <gtest/gtest.h>
#include <vector>

using namespace DeltaEngine;

namespace
{
float GetFloatPropertyValue(Camera& camera, const char* propertyName)
{
    DClass* dclass = camera.GetClass();
    DProperty* property = dclass ? dclass->FindPropertyByName(propertyName) : nullptr;
    EXPECT_NE(property, nullptr);
    return property ? *static_cast<float*>(property->GetValue(&camera)) : 0.0f;
}
}

TEST(RenderCoreTests, CameraUpdatesReflectedProjectionProperties)
{
    Camera camera;

    camera.UpdateParameters(DirectX::XM_PIDIV2, 1.5f, 0.25f, 2048.0f);
    camera.UpdateAspectRatio(2.0f);

    EXPECT_FLOAT_EQ(GetFloatPropertyValue(camera, "m_fov"), DirectX::XM_PIDIV2);
    EXPECT_FLOAT_EQ(GetFloatPropertyValue(camera, "m_aspectRatio"), 2.0f);
    EXPECT_FLOAT_EQ(GetFloatPropertyValue(camera, "m_near"), 0.25f);
    EXPECT_FLOAT_EQ(GetFloatPropertyValue(camera, "m_far"), 2048.0f);
}

TEST(RenderCoreTests, MaterialPreservesShaderTextureStateAndRejectsInvalidIndices)
{
    DShader initialShader;
    DShader replacementShader;
    DTexture albedo;
    DTexture normal;
    DMaterial material;

    material.Initialize(&initialShader);
    EXPECT_EQ(material.GetShader(), &initialShader);

    material.SetShader(&replacementShader);
    material.AddTexture(&albedo);
    material.AddTexture(&normal);

    EXPECT_EQ(material.GetShader(), &replacementShader);
    EXPECT_EQ(material.GetTexture(0), &albedo);
    EXPECT_EQ(material.GetTexture(1), &normal);
    EXPECT_EQ(material.GetTexture(-1), nullptr);
    EXPECT_EQ(material.GetTexture(2), nullptr);
}

TEST(RenderCoreTests, MeshReturnsAssignedMaterialsAndStartsWithoutSubmeshes)
{
    DMaterial firstMaterial;
    DMaterial secondMaterial;
    DMesh mesh;
    std::vector<DMaterial*> materials { &firstMaterial, &secondMaterial };

    EXPECT_EQ(mesh.GetSubMeshCount(), 0);
    EXPECT_TRUE(mesh.GetMaterials().empty());
    EXPECT_EQ(mesh.GetMaterial(), nullptr);

    mesh.SetMaterials(materials);

    ASSERT_EQ(mesh.GetMaterials().size(), 2u);
    EXPECT_EQ(mesh.GetMaterials()[0], &firstMaterial);
    EXPECT_EQ(mesh.GetMaterials()[1], &secondMaterial);
    EXPECT_EQ(mesh.GetMaterial(0), &firstMaterial);
    EXPECT_EQ(mesh.GetMaterial(1), &secondMaterial);
    EXPECT_EQ(mesh.GetMaterial(-1), nullptr);
    EXPECT_EQ(mesh.GetMaterial(2), nullptr);
}

TEST(RenderCoreTests, ShaderOwnsStableInputLayoutSemanticNames)
{
    DShader shader;
    char semanticName[] = "POSITION";

    D3D12_INPUT_ELEMENT_DESC layoutElement {};
    layoutElement.SemanticName = semanticName;
    layoutElement.SemanticIndex = 0;
    layoutElement.Format = DXGI_FORMAT_R32G32B32_FLOAT;
    layoutElement.InputSlot = 0;
    layoutElement.AlignedByteOffset = 0;
    layoutElement.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    layoutElement.InstanceDataStepRate = 0;

    shader.SetInputLayout({ layoutElement });
    semanticName[0] = 'X';

    const auto copiedLayout = shader.GetInputLayout();
    ASSERT_EQ(copiedLayout.size(), 1u);
    EXPECT_STREQ(copiedLayout[0].SemanticName, "POSITION");
    EXPECT_NE(copiedLayout[0].SemanticName, semanticName);
}

TEST(RenderCoreTests, EngineMainCreatesAndCleansTheEditorWorld)
{
    EngineMain engine;

    EXPECT_EQ(engine.GetWorld(), nullptr);
    EXPECT_EQ(engine.GetCamera(), nullptr);

    engine.CreateWorld();

    DWorld* world = engine.GetWorld();
    ASSERT_NE(world, nullptr);
    EXPECT_TRUE(world->GetGameObjects().empty());

    world->CreateGameObject("CleanupProbe");
    EXPECT_EQ(world->GetGameObjects().size(), 1u);

    engine.Cleanup();

    EXPECT_EQ(engine.GetWorld(), nullptr);
    EXPECT_EQ(engine.GetCamera(), nullptr);
}
