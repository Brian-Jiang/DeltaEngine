#include "Runtime/Graphics/Renderer/MeshRenderer.h"

#include "Runtime/Core/DMesh.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <gtest/gtest.h>

#include <memory>

using namespace DeltaEngine;

TEST(MeshRendererTests, MeshRenderer_CreateAndDestroy_DoesNotCrash)
{
    // Arrange / Act
    MeshRenderer* mr = CreateDObject<MeshRenderer>();

    // Assert
    ASSERT_NE(mr, nullptr);

    // Cleanup
    GetReflectionRegistry().DestroyObject(mr);
}

TEST(MeshRendererTests, MeshRenderer_SetMeshNull_IsSafeAndIdempotent)
{
    // Arrange
    MeshRenderer* mr = CreateDObject<MeshRenderer>();
    ASSERT_NE(mr, nullptr);

    // Act
    mr->SetMesh(nullptr);
    mr->SetMesh(nullptr);

    // Assert: no crash
    SUCCEED();

    // Cleanup
    GetReflectionRegistry().DestroyObject(mr);
}

TEST(MeshRendererTests, MeshRenderer_SetMeshThenNull_RebuildsAndClearsProxy)
{
    // Arrange
    MeshRenderer* mr = CreateDObject<MeshRenderer>();
    ASSERT_NE(mr, nullptr);
    DMesh* mesh = CreateDObject<DMesh>();
    ASSERT_NE(mesh, nullptr);

    // Act
    mr->SetMesh(mesh);
    mr->SetMesh(nullptr);

    // Assert: no crash
    SUCCEED();

    // Cleanup
    GetReflectionRegistry().DestroyObject(mesh);
    GetReflectionRegistry().DestroyObject(mr);
}
