#pragma once

#include "UIComponents/ClassPickerPopup.h"
#include "UIComponents/ContextMenuPopup.h"
#include "UIComponents/EditorInlineRename.h"

#include "EngineIncludes.h"

#include <vector>

#include "EditorWindows/EditorWindow.h"

#include "Runtime/Core/UUID.h"

DELTA_ENGINE_NS_BEGIN

class DObject;
class SceneComponent;
class DComponent;

class EditorWindow_ComponentsHierarchy : public EditorWindow
{
public:
    EditorWindow_ComponentsHierarchy();
    ~EditorWindow_ComponentsHierarchy();

    /// Scene tree and non-spatial components for the selection's GameObject.
    void Render(bool& open) override;

private:
    void RenderSceneComponentTree(SceneComponent* sceneComponent);
    void RenderRegularComponents(const std::vector<DComponent*>& components);
    void OnRenameCommitted(DObject* obj);

    ClassPickerPopup   m_addCompPicker;
    ContextMenuPopup   m_destroyCompMenu;
    EditorInlineRename m_inlineRename;
    ObjectId           m_renameComponentId = ObjectId::Null();
};

DELTA_ENGINE_NS_END
