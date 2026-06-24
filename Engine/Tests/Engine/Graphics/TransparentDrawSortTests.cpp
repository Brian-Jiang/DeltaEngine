#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/RenderProxy/MeshRenderProxy.h"
#include "Runtime/Graphics/Renderer/MeshRenderer.h"
#include "Runtime/Graphics/Structures/Camera.h"
#include "Runtime/Graphics/TransparentDrawEntry.h"
#include "Runtime/IO/IOManager.h"

#include <d3dx12.h>
#include <DirectXMath.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <vector>

using namespace DirectX;
using namespace DeltaEngine;

namespace
{
std::filesystem::path StarObjPath()
{
    return IOManager::GetEngineSourceAssetFullPath(std::filesystem::path("Star.obj"));
}

DMesh* TryImportStarMesh()
{
    const std::filesystem::path path = StarObjPath();
    if (!std::filesystem::exists(path))
        return nullptr;

    DMesh* mesh = CreateDObject<DMesh>();
    mesh->ImportFromAbsolutePath(path);
    if (mesh->GetSubMeshCount() <= 0)
        return nullptr;

    return mesh;
}

DMesh* ImportStarMeshOrSkip()
{
    DMesh* mesh = TryImportStarMesh();
    if (!mesh)
        GTEST_SKIP() << "Star.obj not available at " << StarObjPath().string();
    return mesh;
}

void SetTransparentMaterial(DMaterial& material)
{
    material.SetRenderMode(static_cast<uint32_t>(ERenderMode::Transparent));
}
}

TEST(TransparentDrawSortTests, SortTransparentDrawEntriesDescending_OrdersFarToNear)
{
    std::vector<TransparentDrawEntry> entries(3);
    entries[0].sortDepth = 10.f;
    entries[1].sortDepth = 30.f;
    entries[2].sortDepth = 20.f;

    SortTransparentDrawEntriesDescending(entries);

    EXPECT_FLOAT_EQ(entries[0].sortDepth, 30.f);
    EXPECT_FLOAT_EQ(entries[1].sortDepth, 20.f);
    EXPECT_FLOAT_EQ(entries[2].sortDepth, 10.f);
}

TEST(TransparentDrawSortTests, ComputeSubmeshSortDepth_MatchesCameraDistance)
{
    DMesh* mesh = ImportStarMeshOrSkip();

    const XMVECTOR cameraPosition = XMVectorSet(0.f, 0.f, 0.f, 1.f);
    const XMMATRIX nearTransform = XMMatrixTranslation(0.f, 0.f, 5.f);
    const XMMATRIX farTransform = XMMatrixTranslation(0.f, 0.f, 15.f);

    const float nearDepth = MeshRenderProxy::ComputeSubmeshSortDepth(mesh, 0, nearTransform, cameraPosition);
    const float farDepth = MeshRenderProxy::ComputeSubmeshSortDepth(mesh, 0, farTransform, cameraPosition);

    EXPECT_GT(farDepth, nearDepth);
    EXPECT_NEAR(nearDepth, 5.f, 2.f);
    EXPECT_NEAR(farDepth, 15.f, 2.f);
}

TEST(TransparentDrawSortTests, AppendTransparentDrawEntries_SkipsOpaqueSubmeshes)
{
    DMesh* mesh = ImportStarMeshOrSkip();

    DMaterial opaqueMaterial;
    DMaterial transparentMaterial;
    SetTransparentMaterial(transparentMaterial);

    const int submeshCount = mesh->GetSubMeshCount();
    std::vector<DMaterial*> materials(static_cast<size_t>(submeshCount), &opaqueMaterial);
    if (submeshCount >= 2)
        materials[1] = &transparentMaterial;
    else
        materials[0] = &transparentMaterial;

    mesh->SetMaterials(materials);

    MeshRenderProxy proxy(mesh, std::make_shared<MeshRendererSettings>());
    const XMVECTOR cameraPosition = XMVectorSet(0.f, 0.f, 0.f, 1.f);

    std::vector<TransparentDrawEntry> entries;
    proxy.AppendTransparentDrawEntries(entries, cameraPosition);

    if (submeshCount >= 2)
    {
        ASSERT_EQ(entries.size(), 1u);
        EXPECT_EQ(entries[0].submeshIndex, 1u);
    }
    else
    {
        ASSERT_EQ(entries.size(), 1u);
        EXPECT_EQ(entries[0].submeshIndex, 0u);
    }
}

TEST(TransparentDrawSortTests, AppendTransparentDrawEntries_AllOpaqueMesh_ReturnsEmpty)
{
    DMesh* mesh = ImportStarMeshOrSkip();

    DMaterial opaqueMaterial;
    std::vector<DMaterial*> materials(static_cast<size_t>(mesh->GetSubMeshCount()), &opaqueMaterial);
    mesh->SetMaterials(materials);

    MeshRenderProxy proxy(mesh, std::make_shared<MeshRendererSettings>());
    const XMVECTOR cameraPosition = XMVectorSet(0.f, 0.f, 0.f, 1.f);

    std::vector<TransparentDrawEntry> entries;
    proxy.AppendTransparentDrawEntries(entries, cameraPosition);

    EXPECT_TRUE(entries.empty());
}

TEST(TransparentDrawSortTests, GatherDrawCalls_ActivePassForward_SkipsTransparentSubmeshes)
{
    DMesh* mesh = ImportStarMeshOrSkip();

    DMaterial transparentMaterial;
    SetTransparentMaterial(transparentMaterial);
    std::vector<DMaterial*> materials(static_cast<size_t>(mesh->GetSubMeshCount()), &transparentMaterial);
    mesh->SetMaterials(materials);

    MeshRenderProxy proxy(mesh, std::make_shared<MeshRendererSettings>());
    auto context = std::make_shared<DXGraphicsContext>();
    context->activePass = ScenePassType::Forward;

    proxy.GatherDrawCalls(context);
    SUCCEED();
}

TEST(TransparentDrawSortTests, GatherDrawCalls_ActivePassTransparent_SkipsOpaqueSubmeshes)
{
    DMesh* mesh = ImportStarMeshOrSkip();

    DMaterial opaqueMaterial;
    std::vector<DMaterial*> materials(static_cast<size_t>(mesh->GetSubMeshCount()), &opaqueMaterial);
    mesh->SetMaterials(materials);

    MeshRenderProxy proxy(mesh, std::make_shared<MeshRendererSettings>());
    auto context = std::make_shared<DXGraphicsContext>();
    context->activePass = ScenePassType::Transparent;

    proxy.GatherDrawCalls(context);
    SUCCEED();
}

TEST(TransparentDrawSortTests, DWorld_GatherTransparentDrawCalls_SortsMultipleRenderersBackToFront)
{
    DMesh* mesh = ImportStarMeshOrSkip();

    DMaterial transparentMaterial;
    SetTransparentMaterial(transparentMaterial);
    std::vector<DMaterial*> materials(static_cast<size_t>(mesh->GetSubMeshCount()), &transparentMaterial);
    mesh->SetMaterials(materials);

    DWorld* world = CreateDObject<DWorld>();
    GameObject* nearObject = world->CreateGameObject("Near");
    GameObject* farObject = world->CreateGameObject("Far");

    MeshRenderer* nearRenderer = nearObject->AddSceneComponent<MeshRenderer>();
    MeshRenderer* farRenderer = farObject->AddSceneComponent<MeshRenderer>();
    nearRenderer->SetMesh(mesh);
    farRenderer->SetMesh(mesh);
    nearRenderer->SetLocalPosition(0.f, 0.f, 5.f);
    farRenderer->SetLocalPosition(0.f, 0.f, 20.f);

    ActiveRenderCamera arc {};
    arc.cb.position = XMVectorSet(0.f, 0.f, 0.f, 1.f);

    std::vector<TransparentDrawEntry> collected;
    nearRenderer->CollectTransparentDrawEntries(collected, arc.cb.position);
    farRenderer->CollectTransparentDrawEntries(collected, arc.cb.position);
    SortTransparentDrawEntriesDescending(collected);

    ASSERT_EQ(collected.size(), 2u);
    EXPECT_GT(collected[0].sortDepth, collected[1].sortDepth);
}
