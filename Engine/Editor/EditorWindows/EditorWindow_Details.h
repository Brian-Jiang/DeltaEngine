#pragma once

#include "EngineIncludes.h"

#include "EditorWindows/EditorWindow.h"
#include "Runtime/Core/Delegates/DelegateHandle.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DVectorProperty.h"
#include "Runtime/Core/DObject.h"
#include "UIComponents/PropertyWidgets/Vec3Field.h"
#include "UIComponents/PropertyWidgets/ScalarField.h"
#include "UIComponents/PropertyWidgets/ColorField.h"
#include "UIComponents/PropertyWidgets/StringField.h"
#include "UIComponents/PropertyWidgets/ObjectPtrField.h"
#include "UIComponents/PropertyWidgets/ReferenceField.h"
#include "UIComponents/WidgetEditEvent.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class GameObject;
class SceneComponent;
class DComponent;
class DStruct;

class EditorWindow_Details : public EditorWindow
{
public:
    EditorWindow_Details();
    ~EditorWindow_Details();

    /// Property inspector for the current selection (asset, GameObject, or component).
    void Render(bool& open) override;

private:
    void HandleSelectionChanged();

    void RenderAssetDetails(const AssetId& assetId);
    void RenderFolderDetails(const std::string& folderRelPath);
    void RenderGameObjectDetails(GameObject* gameObject);
    void RenderComponentDetails(DComponent* component);
    void RenderSceneComponentTransform(SceneComponent* sceneComponent);

    void DrawPropertyEditor(DObject* instance, DClass* dclass, int depth = 0);
    void RenderSingleProperty(DObject* instance, DProperty* prop, int depth);
    void DrawFunctionButtons(DObject* instance, DClass* dclass);
    void DrawReadOnlyProperty(const std::string& label, const std::string& value) const;
    void DrawVectorElements(const DVectorPropertyBase* vectorProp, void* vectorAddr, DObject* parentObject, int depth);

    WidgetEditEvent DrawIntProperty(DObject* instance, DProperty* prop);
    WidgetEditEvent DrawFloatProperty(DObject* instance, DProperty* prop);
    WidgetEditEvent DrawDoubleProperty(DObject* instance, DProperty* prop);
    WidgetEditEvent DrawBoolProperty(DObject* instance, DProperty* prop);
    WidgetEditEvent DrawStringProperty(DObject* instance, DProperty* prop);
    WidgetEditEvent DrawFilesystemPathProperty(DObject* instance, DProperty* prop);
    WidgetEditEvent DrawVector3Property(DObject* instance, DProperty* prop);
    WidgetEditEvent DrawQuaternionProperty(DObject* instance, DProperty* prop);
    WidgetEditEvent DrawFloat4Property(DObject* instance, DProperty* prop);
    WidgetEditEvent DrawFloat4x4Property(DObject* instance, DProperty* prop);
    bool DrawObjectPtrProperty(DObject* instance, DProperty* prop, int depth);
    bool DrawBulkDataProperty(DObject* instance, DProperty* prop);
    bool DrawVectorProperty(DObject* instance, DProperty* prop, int depth);

    WidgetEditEvent DrawStructPropertyEditor(DObject* instance, DStructProperty* dsp);
    void DrawStructSchemaFields(void* structBase, DStruct* ds, WidgetEditEvent& merged);

    WidgetEditEvent DrawIntPropertyAt(void* container, DProperty* prop);
    WidgetEditEvent DrawFloatPropertyAt(void* container, DProperty* prop);
    WidgetEditEvent DrawDoublePropertyAt(void* container, DProperty* prop);
    WidgetEditEvent DrawBoolPropertyAt(void* container, DProperty* prop);
    WidgetEditEvent DrawStringPropertyAt(void* container, DProperty* prop);
    WidgetEditEvent DrawFilesystemPathPropertyAt(void* container, DProperty* prop);

    const DProperty* m_activeEditProp   = nullptr;
    nlohmann::json   m_activeEditBefore = {};
    DObject*         m_activeEditObject = nullptr;

    bool             m_transformEditing = false;
    SceneComponent*  m_transformEditTarget = nullptr;
    nlohmann::json   m_transformEditBefore = {};

    Vec3Field      m_vec3Field;
    ScalarField    m_scalarField;
    ColorField     m_colorField;
    StringField    m_stringField;
    ReferenceField m_refField;
    ObjectPtrField m_objPtrField;

    FDelegateHandle m_onSelectionChangedHandle;
};

DELTA_ENGINE_NS_END
