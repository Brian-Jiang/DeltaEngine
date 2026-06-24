#include "Shared/GpuReadback.h"
#include "Shared/GpuSceneBuilder.h"
#include "Shared/GpuTestAssetFixture.h"

#include "Runtime/Core/Camera.h"
#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Graphics/Renderer/MeshRenderer.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include <DirectXMath.h>
#include <cmath>
#include <gtest/gtest.h>

using namespace DirectX;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
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
    if (doubleSided)
        SetBoolProperty(*material, "m_doubleSided", true);
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
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    Camera* camera = builder.AddCamera();
    ASSERT_NE(AddTransparentSphere(builder, "TransparentSphere", 0.0f, 0.0f, 0.0f, { 1.0f, 0.0f, 0.0f, 0.7f }), nullptr);
    ASSERT_NE(camera, nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);
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
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    Camera* camera = builder.AddCamera();
    ASSERT_NE(AddTransparentSphere(builder, "NearSphere", 0.0f, 0.0f, -4.0f, { 1.0f, 0.0f, 0.0f, 0.7f }), nullptr);
    ASSERT_NE(AddTransparentSphere(builder, "FarSphere", 0.0f, 0.0f, -12.0f, { 0.0f, 1.0f, 0.0f, 0.7f }), nullptr);
    ASSERT_NE(camera, nullptr);
    ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);
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

    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    Camera* camera = builder.AddCamera();
    ASSERT_NE(AddTransparentSphere(builder, "TransparentSphere", 0.0f, 0.0f, -2.0f, { 1.0f, 1.0f, 1.0f, 0.25f }), nullptr);
    ASSERT_NE(camera, nullptr);
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
