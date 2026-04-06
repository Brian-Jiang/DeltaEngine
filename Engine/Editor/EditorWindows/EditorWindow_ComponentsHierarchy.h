#pragma once

#include "UIComponents/ClassPickerPopup.h"
#include "UIComponents/ContextMenuPopup.h"

#include "EngineIncludes.h"

#include <vector>

#include "EditorWindows/EditorWindow.h"

DELTA_ENGINE_NS_BEGIN

class SceneComponent;
class DComponent;

class EditorWindow_ComponentsHierarchy : public EditorWindow
{
public:
    EditorWindow_ComponentsHierarchy();
    ~EditorWindow_ComponentsHierarchy();

    /// Scene tree and non-spatial components for the selection's GameObject.
    void Render(bool& open) override;

    /// ImGui window title.
    const char* m_title = "Components Hierarchy";

private:
    void RenderSceneComponentTree(SceneComponent* sceneComponent);
    void RenderRegularComponents(const std::vector<DComponent*>& components);

    ClassPickerPopup m_addCompPicker;
    ContextMenuPopup m_destroyCompMenu;
};

DELTA_ENGINE_NS_END
