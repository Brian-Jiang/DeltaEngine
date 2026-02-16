#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <vector>

#include "EditorWindows/EditorWindow.h"

DELTA_ENGINE_NS_BEGIN

class GameObject;
class SceneComponent;
class DComponent;

class EditorWindow_ComponentsHierarchy : public EditorWindow
{
public:
    EditorWindow_ComponentsHierarchy();
    ~EditorWindow_ComponentsHierarchy();

    void Render() override;

    const char* m_title = "Components Hierarchy";
    bool* m_open = nullptr;

private:
    void RenderSceneComponentTree(std::shared_ptr<SceneComponent> sceneComponent);
    void RenderRegularComponents(const std::vector<std::shared_ptr<DComponent>>& components);
};

DELTA_ENGINE_NS_END
