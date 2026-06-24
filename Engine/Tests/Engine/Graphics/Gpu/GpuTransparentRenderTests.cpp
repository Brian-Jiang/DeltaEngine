#include "Shared/GpuReadback.h"
#include "Shared/GpuSceneBuilder.h"
#include "Shared/GpuTestAssetFixture.h"

#include "Runtime/Assets/PA_CommonAssets.h"
#include "Runtime/Core/Camera.h"
#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/DShader.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/Light/DirectionalLight.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Graphics/Renderer/MeshRenderer.h"
#include "Runtime/IO/IOManager.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <DirectXMath.h>
#include <SimpleMath.h>
#include <cmath>
#include <filesystem>
#include <gtest/gtest.h>
#include <vector>

using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
std::filesystem::path StarObjPath()
{
    return IOManager::GetEngineSourceAssetFullPath(std::filesystem::path("Star.obj"));
}

DMesh* ImportIndependentStarMesh()
{
    const std::filesystem::path path = StarObjPath();
    if (!std::filesystem::exists(path))
        return nullptr;

    DMesh* mesh = CreateDObject<DMesh>();
    mesh->ImportFromAbsolutePath(path);
    return mesh->GetSubMeshCount() > 0 ? mesh : nullptr;
}

DShader* LoadSpherePbrShader(EditorAssetDatabase& assetDatabase)
{
    const std::filesystem::path assetPath = std::filesystem::weakly_canonical(
        IOManager::GetEngineImportedAssetFullPath("Sphere/SphereMesh"));
    const AssetId id = assetDatabase.FindAssetIdByPath(assetPath);
    if (id.IsNull())
        return nullptr;

    PA_StaticMesh* staticMeshAsset = assetDatabase.LoadAsset<PA_StaticMesh>(id);
    if (!staticMeshAsset)
        return nullptr;

    DMesh* mesh = staticMeshAsset->GetStaticMesh();
    if (!mesh)
        return nullptr;

    DMaterial* material = mesh->GetMaterial(0);
    return material ? material->GetShader() : nullptr;
}

void SetBoolProperty(DObject& obj, const char* name, const bool value)
{
    DClass* cls = obj.GetClass();
    ASSERT_NE(cls, nullptr);
    DProperty* prop = cls->FindPropertyByName(name);
    ASSERT_NE(prop, nullptr);
    prop->SetValue(&obj, const_cast<bool*>(&value));
}

void SetFloat4Property(DObject& obj, const char* name, const XMFLOAT4& value)
{
    DClass* cls = obj.GetClass();
    ASSERT_NE(cls, nullptr);
    DProperty* prop = cls->FindPropertyByName(name);
    ASSERT_NE(prop, nullptr);
    prop->SetValue(&obj, const_cast<XMFLOAT4*>(&value));
}

void SetFloatProperty(DObject& obj, const char* name, const float value)
{
    DClass* cls = obj.GetClass();
    ASSERT_NE(cls, nullptr);
    DProperty* prop = cls->FindPropertyByName(name);
    ASSERT_NE(prop, nullptr);
    prop->SetValue(&obj, const_cast<float*>(&value));
}

DMesh* GetRendererMesh(MeshRenderer* renderer)
{
    if (!renderer)
        return nullptr;

    DClass* cls = renderer->GetClass();
    if (!cls)
        return nullptr;

    DProperty* meshProp = cls->FindPropertyByName("m_mesh");
    if (!meshProp)
        return nullptr;

    void* value = meshProp->GetValue(renderer);
    return value ? *static_cast<DMesh**>(value) : nullptr;
}

DMaterial* GetMeshMaterial(MeshRenderer* renderer)
{
    DMesh* mesh = GetRendererMesh(renderer);
    return mesh ? mesh->GetMaterial(0) : nullptr;
}

void ConfigureTransparentMaterial(DMaterial* material, const XMFLOAT4& baseColor, const bool doubleSided = false)
{
    ASSERT_NE(material, nullptr);
    material->SetRenderMode(static_cast<uint32_t>(ERenderMode::Transparent));
    SetFloat4Property(*material, "m_baseColor", baseColor);
    SetFloatProperty(*material, "m_metallic", 0.0f);
    SetFloatProperty(*material, "m_roughness", 0.5f);
    if (doubleSided)
        SetBoolProperty(*material, "m_doubleSided", true);
}

DMaterial* CreateUniqueTransparentMaterial(EditorAssetDatabase& assetDatabase, const XMFLOAT4& baseColor)
{
    DShader* shader = LoadSpherePbrShader(assetDatabase);
    if (!shader)
        return nullptr;

    DMaterial* material = CreateDObject<DMaterial>();
    if (!material)
        return nullptr;

    material->Initialize(shader);
    ConfigureTransparentMaterial(material, baseColor);
    return material;
}

void AssignUniqueTransparentMaterial(DMesh* mesh, DMaterial* material)
{
    ASSERT_NE(mesh, nullptr);
    ASSERT_NE(material, nullptr);
    std::vector<DMaterial*> materials(static_cast<size_t>(mesh->GetSubMeshCount()), material);
    mesh->SetMaterials(materials);
}

MeshRenderer* AddIndependentTransparentStar(EditorAssetDatabase& assetDatabase,
    DWorld* world,
    DScene* scene,
    const char* name,
    const float x,
    const float y,
    const float z,
    const XMFLOAT4& baseColor)
{
    DMesh* mesh = ImportIndependentStarMesh();
    DMaterial* material = CreateUniqueTransparentMaterial(assetDatabase, baseColor);
    if (!mesh || !material || !world)
        return nullptr;

    AssignUniqueTransparentMaterial(mesh, material);

    GameObject* gameObject = world->CreateGameObjectInScene(scene, name);
    if (!gameObject)
        return nullptr;

    MeshRenderer* renderer = gameObject->AddSceneComponent<MeshRenderer>();
    if (!renderer)
        return nullptr;

    renderer->SetLocalPosition(x, y, z);
    renderer->SetMesh(mesh);
    return renderer;
}

DirectionalLight* AddCameraFacingDirectionalLight(GpuSceneBuilder& builder)
{
    DirectionalLight* light = builder.AddDirectionalLightWithShadows();
    if (!light)
        return nullptr;

    light->SetLocalRotation(Quaternion::Identity);
    return light;
}

void AttachPostProcessStack(GpuSceneBuilder& builder, Camera* camera)
{
    PostProcessStack* stack = builder.CreatePassthroughTonemapStack();
    ASSERT_NE(stack, nullptr);
    builder.AttachPostProcessStack(camera, stack);
}

MeshRenderer* AddTransparentSphere(GpuSceneBuilder& builder,
    const char* name,
    const float x,
    const float y,
    const float z,
    const XMFLOAT4& baseColor,
    const bool doubleSided = false)
{
    MeshRenderer* renderer = builder.AddSphereMesh(name, x, y, z);
    if (!renderer)
        return nullptr;

    DMaterial* material = GetMeshMaterial(renderer);
    ConfigureTransparentMaterial(material, baseColor, doubleSided);
    return renderer;
}

uint32_t GetSceneColorSampleCount(const DXRenderManager& renderManager)
{
    const std::shared_ptr<RenderTarget> renderTarget = renderManager.GetRenderTarget();
    if (!renderTarget)
        return 1u;

    const std::shared_ptr<DirectX12Texture> sceneColor = renderTarget->GetTexture(AttachmentPoint::Color0);
    if (!sceneColor)
        return 1u;

    return sceneColor->GetD3D12ResourceDesc().SampleDesc.Count;
}
} // namespace

class GpuTransparentRenderTests : public GpuTestAssetFixture
{
};

class GpuDeferredTransparentRenderTests : public GpuTestAssetFixture
{
protected:
    RenderPath GetInitialRenderPath() const override { return RenderPath::Deferred; }
};

TEST_F(GpuTransparentRenderTests, Forward_TransparentSphere_RendersWithCleanValidation)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(AddTransparentSphere(builder, "TransparentSphere", 0.0f, 0.0f, 0.0f, { 1.0f, 0.0f, 0.0f, 0.7f }), nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);

    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuDeferredTransparentRenderTests, Deferred_TransparentSphere_RendersWithCleanValidation)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(AddTransparentSphere(builder, "TransparentSphere", 0.0f, 0.0f, 0.0f, { 1.0f, 0.0f, 0.0f, 0.7f }), nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);

    builder.InitGpuResources();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuTransparentRenderTests, TransparentSphere_ProducesNonClearCenterPixel)
{
    if (!std::filesystem::exists(StarObjPath()))
        GTEST_SKIP() << "Star.obj not available at " << StarObjPath().string();

    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    DWorld* world = GetEngine().GetWorld();
    Camera* camera = builder.AddCamera();
    ASSERT_NE(AddIndependentTransparentStar(builder, GetAssetDatabase(), world, world->GetActiveScene(),
                 "TransparentStar", 0.0f, 0.0f, 0.0f, { 1.0f, 0.0f, 0.0f, 0.7f }),
        nullptr);
    ASSERT_NE(camera, nullptr);
    ASSERT_NE(AddCameraFacingDirectionalLight(builder), nullptr);
    AttachPostProcessStack(builder, camera);

    builder.InitGpuResources();
    RenderSceneFrame();

    const std::shared_ptr<DirectX12Texture> finalColor = GetFinalColorTexture();
    ASSERT_NE(finalColor, nullptr);
    EXPECT_TRUE(TextureHasNonClearContent(
        *GetDevice(),
        GetDirectQueue(),
        finalColor,
        0.0f,
        0.2f,
        0.4f,
        0.01f));
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuTransparentRenderTests, TwoTransparentSpheres_NearRedDominatesCenterPixel)
{
    if (!std::filesystem::exists(StarObjPath()))
        GTEST_SKIP() << "Star.obj not available at " << StarObjPath().string();

    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    DWorld* world = GetEngine().GetWorld();
    DScene* scene = world ? world->GetActiveScene() : nullptr;
    Camera* camera = builder.AddCamera();
    ASSERT_NE(AddIndependentTransparentStar(builder, GetAssetDatabase(), world, scene, "NearStar", 0.0f, 0.0f, -4.0f,
                 { 1.0f, 0.0f, 0.0f, 0.7f }),
        nullptr);
    ASSERT_NE(AddIndependentTransparentStar(builder, GetAssetDatabase(), world, scene, "FarStar", 0.0f, 0.0f, -12.0f,
                 { 0.0f, 1.0f, 0.0f, 0.7f }),
        nullptr);
    ASSERT_NE(camera, nullptr);
    ASSERT_NE(AddCameraFacingDirectionalLight(builder), nullptr);
    AttachPostProcessStack(builder, camera);

    builder.InitGpuResources();
    RenderSceneFrame();

    const std::shared_ptr<DirectX12Texture> finalColor = GetFinalColorTexture();
    ASSERT_NE(finalColor, nullptr);

    GpuReadbackPixel centerPixel{};
    ASSERT_TRUE(ReadTextureCenterPixel(*GetDevice(), GetDirectQueue(), finalColor, centerPixel));
    EXPECT_GT(centerPixel.r, centerPixel.g + 0.01f);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuTransparentRenderTests, DoubleSidedTransparentSphere_RendersWithCleanValidation)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(AddTransparentSphere(builder, "DoubleSidedSphere", 0.0f, 0.0f, 0.0f, { 0.2f, 0.8f, 1.0f, 0.6f }, true),
        nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);

    builder.InitGpuResources();
    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuTransparentRenderTests, TransparentSphereWithSkybox_StillShowsNonClearBackground)
{
    PA_DScene* sceneAsset = LoadImportedScene("DefaultScene");
    ASSERT_NE(sceneAsset, nullptr);

    DWorld* world = GetEngine().GetWorld();
    ASSERT_NE(world, nullptr);

    Camera* camera = nullptr;
    if (DScene* scene = world->GetActiveScene())
    {
        for (GameObject* gameObject : scene->GetGameObjects())
        {
            if (gameObject && (camera = gameObject->GetRootSceneComponent<Camera>()))
                break;
        }
    }
    ASSERT_NE(camera, nullptr);

    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(AddTransparentSphere(builder, "TransparentSphere", 0.0f, 0.0f, -2.0f, { 1.0f, 1.0f, 1.0f, 0.25f }), nullptr);
    AttachPostProcessStack(builder, camera);

    builder.InitGpuResources();
    RenderSceneFrame();

    const std::shared_ptr<DirectX12Texture> finalColor = GetFinalColorTexture();
    ASSERT_NE(finalColor, nullptr);
    EXPECT_TRUE(TextureHasNonClearContent(
        *GetDevice(),
        GetDirectQueue(),
        finalColor,
        0.0f,
        0.2f,
        0.4f,
        0.01f));
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

// BuildForwardFrameGraph runs Transparent before MsaaResolve (see FullChain_WithMsaaResolve).
TEST_F(GpuTransparentRenderTests, ForwardMsaa_TransparentSphere_RendersOrSkips)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NE(AddTransparentSphere(builder, "TransparentSphere", 0.0f, 0.0f, 0.0f, { 1.0f, 0.0f, 0.0f, 0.7f }), nullptr);
    ASSERT_NE(builder.AddCamera(), nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);

    builder.InitGpuResources();

    if (GetSceneColorSampleCount(GetRenderManager()) <= 1u)
        GTEST_SKIP() << "MSAA not active at current sample count";

    RenderSceneFrame();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}
