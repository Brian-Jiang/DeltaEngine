#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "EditorWindows/EditorWindow.h"

DELTA_ENGINE_NS_BEGIN

class GameObject;
class SceneComponent;
class DComponent;

class EditorWindow_Details : public EditorWindow
{
public:
    EditorWindow_Details();
    ~EditorWindow_Details();

    void Render() override;

    const char* m_title = "Details";
    bool* m_open = nullptr;

private:
    void RenderGameObjectDetails(GameObject* gameObject);
    void RenderComponentDetails(DComponent* component);
    void RenderSceneComponentTransform(SceneComponent* sceneComponent);
};

DELTA_ENGINE_NS_END
