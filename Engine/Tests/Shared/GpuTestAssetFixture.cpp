#include "Shared/GpuTestAssetFixture.h"

#include "Runtime/Core/Camera.h"
#include "Runtime/Core/DScene.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/Structures/Camera.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include <DirectXMath.h>

namespace DeltaEngine::Tests
{
namespace
{
float ReadFloatProperty(const DObject& obj, const char* name)
{
    DClass* cls = obj.GetClass();
    if (!cls)
        return 0.0f;

    DProperty* prop = cls->FindPropertyByName(name);
    if (!prop)
        return 0.0f;

    void* value = prop->GetValue(const_cast<DObject*>(&obj));
    return value ? *static_cast<float*>(value) : 0.0f;
}

Camera* FindFirstSceneCamera(DWorld* world)
{
    if (!world)
        return nullptr;

    auto findInGameObjects = [](const std::vector<GameObject*>& gameObjects) -> Camera*
    {
        for (GameObject* gameObject : gameObjects)
        {
            if (gameObject)
            {
                if (Camera* camera = gameObject->GetRootSceneComponent<Camera>())
                    return camera;
            }
        }
        return nullptr;
    };

    if (DScene* scene = world->GetActiveScene())
    {
        if (Camera* camera = findInGameObjects(scene->GetGameObjects()))
            return camera;
    }

    return findInGameObjects(world->GetGameObjects());
}

ActiveRenderCamera BuildActiveRenderCamera(const Camera& camera, const float renderWidth, const float renderHeight)
{
    using namespace DirectX;

    const float aspectRatio = renderHeight > 0.0f ? renderWidth / renderHeight : 1.0f;
    const float fov = ReadFloatProperty(camera, "m_fov");
    const float nearPlane = ReadFloatProperty(camera, "m_near");
    const float farPlane = ReadFloatProperty(camera, "m_far");

    const XMMATRIX worldMatrix = camera.GetWorldTransform();
    const XMVECTOR forward = worldMatrix.r[2];
    const XMVECTOR up = worldMatrix.r[1];
    const XMVECTOR position = worldMatrix.r[3];

    ActiveRenderCamera arc{};
    arc.fovY = fov;
    arc.nearPlane = nearPlane;
    arc.farPlane = farPlane;
    arc.aspectRatio = aspectRatio;
    arc.cb.viewMatrix = XMMatrixTranspose(XMMatrixLookToLH(position, forward, up));
    arc.cb.projectionMatrix =
        XMMatrixTranspose(XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane));
    arc.cb.position = position;
    return arc;
}
} // namespace

DXRenderManager& GpuTestAssetFixture::GetRenderManager()
{
    auto manager = m_engine->GetRenderManager();
    EXPECT_NE(manager, nullptr);
    return *manager;
}

AssetId GpuTestAssetFixture::FindImportedAssetId(const std::string_view relativePath) const
{
    const std::filesystem::path path =
        std::filesystem::weakly_canonical(IOManager::GetEngineImportedAssetFullPath(relativePath));
    return m_assetDatabase->FindAssetIdByPath(path);
}

void GpuTestAssetFixture::SubmitAndFlush()
{
    const uint64_t fenceValue = GetDevice()->GetFrameFenceValue();
    GetDirectQueue().WaitForFenceValue(fenceValue);
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());
}

PA_DScene* GpuTestAssetFixture::LoadImportedScene(const std::string_view relativePath)
{
    const AssetId sceneId = FindImportedAssetId(relativePath);
    if (sceneId.IsNull())
    {
        ADD_FAILURE() << "Imported scene not found: " << relativePath;
        return nullptr;
    }

    PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(sceneId);
    if (!sceneAsset)
    {
        ADD_FAILURE() << "Failed to load imported scene asset: " << relativePath;
        return nullptr;
    }

    m_engine->LoadScene(sceneAsset->GetAssetId());

    GetDevice()->Flush();
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());
    return sceneAsset;
}

void GpuTestAssetFixture::RenderSceneFrames(const uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        RenderSceneFrame();
        AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());
    }
}

std::shared_ptr<DirectX12Texture> GpuTestAssetFixture::GetFinalColorTexture() const
{
    const DXRenderManager* renderManager = m_engine ? m_engine->GetRenderManager().get() : nullptr;
    if (!renderManager)
        return nullptr;

    if (std::shared_ptr<DirectX12Texture> postProcessTexture = renderManager->GetFinalPostProcessTexture())
        return postProcessTexture;

    const std::shared_ptr<RenderTarget> renderTarget = GetRenderTarget();
    if (!renderTarget)
        return nullptr;

    return renderTarget->GetTexture(AttachmentPoint::Color0);
}

void GpuTestAssetFixture::ReinitializeRenderPipeline()
{
    m_engine->Cleanup();
    m_engine->CreateWorld();

    auto renderManager = CreateRenderManager();
    m_engine->Initialize(renderManager);
    m_engine->GetRenderManager()->InitWorldRenderers(*m_engine->GetWorld());

    GetDevice()->Flush();
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());
}

void GpuTestAssetFixture::RenderSceneFrame()
{
    DXRenderManager& renderManager = GetRenderManager();

    if (Camera* sceneCamera = FindFirstSceneCamera(m_engine->GetWorld()))
    {
        const ActiveRenderCamera arc = BuildActiveRenderCamera(
            *sceneCamera, static_cast<float>(renderManager.GetWidth()), static_cast<float>(renderManager.GetHeight()));
        renderManager.SetPendingActiveRenderCamera(arc);
    }

    renderManager.PrepareFrame();
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());

    renderManager.RenderScene([this](const std::shared_ptr<DXGraphicsContext>& context)
    {
        m_engine->RecordSceneDraws(context);
    });
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());

    renderManager.RenderFrame();
    SubmitAndFlush();
}

void GpuTestAssetFixture::SetUp()
{
    const std::filesystem::path importedRoot = IOManager::GetEngineImportedAssetsFolder();
    if (!std::filesystem::is_directory(importedRoot))
        GTEST_SKIP() << "Engine imported assets folder unavailable: " << importedRoot.string();

    m_assetDatabase = std::make_unique<EditorAssetDatabase>();
    m_locatorScope = std::make_unique<ScopedAssetDatabaseLocatorRegistration>(m_assetDatabase.get());
    m_assetDatabase->ScanAssetsFolder(importedRoot);

    m_engine = std::make_unique<EngineMain>();
    m_engine->CreateWorld();

    auto renderManager = CreateRenderManager();
    m_engine->Initialize(renderManager);
    m_engine->GetRenderManager()->InitWorldRenderers(*m_engine->GetWorld());

    GetDevice()->Flush();
    AssertGpuValidationClean(GetDevice()->GetD3D12Device().Get());
}

void GpuTestAssetFixture::TearDown()
{
    if (m_engine)
        m_engine->Cleanup();

    m_locatorScope.reset();
    m_assetDatabase.reset();
    m_engine.reset();

    GpuGraphicsFixture::TearDown();
}

} // namespace DeltaEngine::Tests
