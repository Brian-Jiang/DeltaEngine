#include "Shared/GpuReadback.h"
#include "Shared/GpuTestAssetFixture.h"

#include "Runtime/Core/Camera.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/Skybox.h"
#include "Runtime/Graphics/RenderProxy/SkyboxRenderProxy.h"
#include "Runtime/Graphics/Light/DirectionalLight.h"
#include "Runtime/Graphics/Light/PointLight.h"
#include "Runtime/Graphics/Light/SpotLight.h"
#include "Runtime/Graphics/Renderer/MeshRenderer.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

// Disabled: this test asserts the exact composition of the DefaultScene placeholder
// asset (game object count, specific shadow-casting lights, skybox). Those assets are
// placeholders, not permanent engine assets, so the assertions are not stable.
#if 0
namespace
{
bool GetCastShadowFlag(DObject* object)
{
    if (!object)
        return false;

    DClass* dclass = object->GetClass();
    if (!dclass)
        return false;

    DProperty* castShadowProp = dclass->FindPropertyByName("m_castShadow");
    if (!castShadowProp)
        return false;

    void* value = castShadowProp->GetValue(object);
    return value && *static_cast<bool*>(value);
}

uint32_t CountShadowCastingMeshRenderers(const DScene* scene)
{
    if (!scene)
        return 0;

    uint32_t count = 0;
    for (GameObject* gameObject : scene->GetGameObjects())
    {
        if (!gameObject)
            continue;

        for (SceneComponent* sceneComponent : gameObject->GetSceneComponents())
        {
            if (auto* meshRenderer = dynamic_cast<MeshRenderer*>(sceneComponent))
            {
                if (GetCastShadowFlag(meshRenderer))
                    ++count;
            }
        }
    }

    return count;
}

template <typename TLight>
uint32_t CountShadowCastingLights(const DScene* scene)
{
    if (!scene)
        return 0;

    uint32_t count = 0;
    for (GameObject* gameObject : scene->GetGameObjects())
    {
        if (!gameObject)
            continue;

        if (auto* light = dynamic_cast<TLight*>(gameObject->GetRootSceneComponent()))
        {
            if (GetCastShadowFlag(light))
                ++count;
        }
    }

    return count;
}

bool SceneHasCamera(const DScene* scene)
{
    if (!scene)
        return false;

    for (GameObject* gameObject : scene->GetGameObjects())
    {
        if (gameObject && gameObject->GetRootSceneComponent<Camera>())
            return true;
    }

    return false;
}
} // namespace

class GpuDefaultSceneTests : public GpuTestAssetFixture
{
};

TEST_F(GpuDefaultSceneTests, DefaultScene_LoadAndRenderThreeFrames_EndToEnd)
{
    PA_DScene* sceneAsset = LoadImportedScene("DefaultScene");
    ASSERT_NE(sceneAsset, nullptr);

    DWorld* world = GetEngine().GetWorld();
    ASSERT_NE(world, nullptr);

    DScene* scene = world->GetActiveScene();
    ASSERT_NE(scene, nullptr);
    EXPECT_EQ(scene, sceneAsset->GetScene());
    EXPECT_EQ(scene->GetGameObjects().size(), 8u);

    Skybox* skybox = world->GetSkybox();
    ASSERT_NE(skybox, nullptr);
    EXPECT_NE(skybox->m_cubemapTexture, nullptr);
    EXPECT_NE(skybox->m_material, nullptr);

    const std::shared_ptr<SkyboxRenderProxy>& skyboxProxy = skybox->GetRenderProxy();
    ASSERT_NE(skyboxProxy, nullptr);
    EXPECT_NE(skyboxProxy->GetGpuCubemap(), nullptr);

    EXPECT_TRUE(SceneHasCamera(scene));
    EXPECT_GE(CountShadowCastingMeshRenderers(scene), 1u);
    EXPECT_EQ(CountShadowCastingLights<DirectionalLight>(scene), 1u);
    EXPECT_EQ(CountShadowCastingLights<PointLight>(scene), 1u);
    EXPECT_EQ(CountShadowCastingLights<SpotLight>(scene), 1u);
    EXPECT_NE(GetRenderManager().GetShadowDepthPSO(), nullptr);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    RenderSceneFrames(3);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

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
#endif
