#pragma once

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/EngineMain.h"

namespace DeltaEngine
{
class Camera;
class DMesh;
class DScene;
class DWorld;
class DirectionalLight;
class MeshRenderer;
class PA_StaticMesh;
class PointLight;
class PostProcessStack;
class SpotLight;
} // namespace DeltaEngine

namespace DeltaEngine::Tests
{

DMesh* GetRendererMesh(MeshRenderer* renderer);
/** World-space bounding radius of the renderer's mesh (uniform scale assumed). */
float MeshBoundingRadius(MeshRenderer* renderer);

class GpuSceneBuilder
{
public:
    GpuSceneBuilder(EngineMain& engine, EditorAssetDatabase& assetDatabase);

    MeshRenderer* AddSphereMesh(const char* name = "Sphere", float x = 0.0f, float y = 0.0f, float z = 0.0f);
    /** Opaque sphere at world radius 1 (the shared source mesh has radius 100 and a Transparent material). */
    MeshRenderer* AddOpaqueUnitSphere(const char* name, float x = 0.0f, float y = 0.0f, float z = 0.0f);
    Camera* AddCamera(float aspectRatio = 1.0f);
    bool AddDefaultSkybox();
    DirectionalLight* AddDirectionalLightWithShadows();
    PointLight* AddPointLightWithShadows();
    SpotLight* AddSpotLightWithShadows();
    PostProcessStack* CreatePassthroughTonemapStack();
    PostProcessStack* CreateBloomTonemapStack();
    PostProcessStack* CreateFullPostProcessStack();
    void AttachPostProcessStack(Camera* camera, PostProcessStack* stack);

    void InitGpuResources();

private:
    PA_StaticMesh* LoadSphereMesh();

    EngineMain& m_engine;
    EditorAssetDatabase& m_assetDatabase;
    DWorld* m_world = nullptr;
    DScene* m_scene = nullptr;
};

} // namespace DeltaEngine::Tests
