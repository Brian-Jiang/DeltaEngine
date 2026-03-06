#pragma once

#include "UIComponents/ClassPickerPopup.h"
#include "UIComponents/ContextMenuPopup.h"

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
    void RenderSceneComponentTree(SceneComponent* sceneComponent);
    void RenderRegularComponents(const std::vector<DComponent*>& components);

    ClassPickerPopup m_addCompPicker;
    ContextMenuPopup m_destroyCompMenu;
};

DELTA_ENGINE_NS_END
