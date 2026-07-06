#include "Shared/GpuSceneBuilder.h"

#include "Runtime/Assets/PA_CommonAssets.h"
#include "Runtime/Core/Camera.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/Light/DirectionalLight.h"
#include "Runtime/Graphics/Light/PointLight.h"
#include "Runtime/Graphics/Light/SpotLight.h"
#include "Runtime/Graphics/PostProcess/BloomPass.h"
#include "Runtime/Graphics/PostProcess/ColorGradingPass.h"
#include "Runtime/Graphics/PostProcess/PassthroughPass.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Graphics/PostProcess/TonemapPass.h"
#include "Runtime/Graphics/PostProcess/VignettePass.h"
#include "Runtime/Graphics/Renderer/MeshRenderer.h"
#include "Runtime/IO/IOManager.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <filesystem>

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace DeltaEngine::Tests
{

GpuSceneBuilder::GpuSceneBuilder(EngineMain& engine, EditorAssetDatabase& assetDatabase)
    : m_engine(engine)
    , m_assetDatabase(assetDatabase)
{
    m_world = m_engine.GetWorld();
    if (m_world)
        m_scene = m_world->GetActiveScene();
}

PA_StaticMesh* GpuSceneBuilder::LoadSphereMesh()
{
    const std::filesystem::path assetPath = std::filesystem::weakly_canonical(
        IOManager::GetEngineImportedAssetFullPath("Sphere/SphereMesh"));
    const AssetId id = m_assetDatabase.FindAssetIdByPath(assetPath);
    if (id.IsNull())
        return nullptr;

    return m_assetDatabase.LoadAsset<PA_StaticMesh>(id);
}

MeshRenderer* GpuSceneBuilder::AddSphereMesh(const char* name, const float x, const float y, const float z)
{
    if (!m_world)
        return nullptr;

    PA_StaticMesh* paStaticMesh = LoadSphereMesh();
    if (!paStaticMesh)
        return nullptr;

    GameObject* go = m_world->CreateGameObjectInScene(m_scene, name);
    if (!go)
        return nullptr;

    MeshRenderer* meshRenderer = go->AddSceneComponent<MeshRenderer>();
    if (!meshRenderer)
        return nullptr;

    meshRenderer->SetLocalPosition(x, y, z);
    meshRenderer->SetMesh(paStaticMesh->GetStaticMesh());
    return meshRenderer;
}

Camera* GpuSceneBuilder::AddCamera(const float aspectRatio)
{
    if (!m_world)
        return nullptr;

    GameObject* cameraGo = m_world->CreateGameObjectInScene(m_scene, "Camera");
    if (!cameraGo)
        return nullptr;

    Camera* camera = cameraGo->AddSceneComponent<Camera>();
    if (!camera)
        return nullptr;

    camera->SetLocalPosition(0.0f, 0.0f, -8.0f);
    camera->UpdateParameters(XM_PIDIV4, aspectRatio, 0.1f, 100.0f);
    return camera;
}

DirectionalLight* GpuSceneBuilder::AddDirectionalLightWithShadows()
{
    if (!m_world)
        return nullptr;

    GameObject* lightGo = m_world->CreateGameObjectInScene(m_scene, "DirectionalLight");
    if (!lightGo)
        return nullptr;

    DirectionalLight* light = lightGo->AddSceneComponent<DirectionalLight>();
    if (!light)
        return nullptr;

    light->SetLocalRotation(Quaternion::CreateFromAxisAngle(Vector3::UnitX, XM_PIDIV4));
    light->UpdateParameters(XMVectorSet(1.0f, 1.0f, 0.95f, 1.0f), 0.7f);
    return light;
}

PointLight* GpuSceneBuilder::AddPointLightWithShadows()
{
    if (!m_world)
        return nullptr;

    GameObject* lightGo = m_world->CreateGameObjectInScene(m_scene, "PointLight");
    if (!lightGo)
        return nullptr;

    PointLight* light = lightGo->AddSceneComponent<PointLight>();
    if (!light)
        return nullptr;

    light->SetLocalPosition(0.0f, 3.0f, 2.0f);
    light->UpdateParameters(XMVectorSet(1.0f, 0.4f, 0.2f, 1.0f), 2.0f, 15.0f);
    return light;
}

SpotLight* GpuSceneBuilder::AddSpotLightWithShadows()
{
    if (!m_world)
        return nullptr;

    GameObject* lightGo = m_world->CreateGameObjectInScene(m_scene, "SpotLight");
    if (!lightGo)
        return nullptr;

    SpotLight* light = lightGo->AddSceneComponent<SpotLight>();
    if (!light)
        return nullptr;

    light->SetLocalPosition(-2.0f, 4.0f, 0.0f);
    light->SetLocalRotation(Quaternion::CreateFromAxisAngle(Vector3::UnitZ, -XM_PIDIV4));
    light->UpdateParameters(
        XMVectorSet(0.2f, 0.8f, 1.0f, 1.0f), 3.0f, 20.0f, XM_PI / 6.0f, XM_PI / 3.0f);
    return light;
}

PostProcessStack* GpuSceneBuilder::CreateBloomTonemapStack()
{
    PostProcessStack* stack = CreateDObject<PostProcessStack>();
    if (!stack)
        return nullptr;

    PostProcessPass* bloom = CreateDObject<BloomPass>();
    PostProcessPass* tonemap = CreateDObject<TonemapPass>();
    if (!bloom || !tonemap)
        return stack;

    stack->m_passes.push_back(bloom);
    stack->m_passes.push_back(tonemap);
    return stack;
}

PostProcessStack* GpuSceneBuilder::CreatePassthroughTonemapStack()
{
    PostProcessStack* stack = CreateDObject<PostProcessStack>();
    if (!stack)
        return nullptr;

    PostProcessPass* passthrough = CreateDObject<PassthroughPass>();
    PostProcessPass* tonemap = CreateDObject<TonemapPass>();
    if (!passthrough || !tonemap)
        return stack;

    stack->m_passes.push_back(passthrough);
    stack->m_passes.push_back(tonemap);
    return stack;
}

PostProcessStack* GpuSceneBuilder::CreateFullPostProcessStack()
{
    PostProcessStack* stack = CreateDObject<PostProcessStack>();
    if (!stack)
        return nullptr;

    PostProcessPass* colorGrading = CreateDObject<ColorGradingPass>();
    PostProcessPass* tonemap = CreateDObject<TonemapPass>();
    PostProcessPass* vignette = CreateDObject<VignettePass>();
    if (!colorGrading || !tonemap || !vignette)
        return stack;

    stack->m_passes.push_back(colorGrading);
    stack->m_passes.push_back(tonemap);
    stack->m_passes.push_back(vignette);
    return stack;
}

void GpuSceneBuilder::AttachPostProcessStack(Camera* camera, PostProcessStack* stack)
{
    if (!camera)
        return;

    DClass* cls = camera->GetClass();
    if (!cls)
        return;

    DProperty* prop = cls->FindPropertyByName("m_postProcessStack");
    if (!prop)
        return;

    auto* ptrProp = dynamic_cast<DObjectPtrPropertyBase*>(prop);
    if (!ptrProp)
        return;

    ptrProp->ResolvePointer(ptrProp->GetValue(camera), stack);
}

void GpuSceneBuilder::InitGpuResources()
{
    if (!m_world)
        return;

    std::shared_ptr<DXRenderManager> renderManager = m_engine.GetRenderManager();
    if (!renderManager)
        return;

    std::shared_ptr<Device> device = renderManager->GetDevice();
    if (device)
        device->Flush();

    renderManager->InitWorldRenderers(*m_world);

    if (device)
        device->Flush();
}

} // namespace DeltaEngine::Tests
