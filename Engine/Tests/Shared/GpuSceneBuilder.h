#pragma once

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Runtime/EngineMain.h"

namespace DeltaEngine
{
class Camera;
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

class GpuSceneBuilder
{
public:
    GpuSceneBuilder(EngineMain& engine, EditorAssetDatabase& assetDatabase);

    MeshRenderer* AddSphereMesh(const char* name = "Sphere", float x = 0.0f, float y = 0.0f, float z = 0.0f);
    Camera* AddCamera(float aspectRatio = 1.0f);
    DirectionalLight* AddDirectionalLightWithShadows();
    PointLight* AddPointLightWithShadows();
    SpotLight* AddSpotLightWithShadows();
    PostProcessStack* CreatePassthroughTonemapStack();
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
