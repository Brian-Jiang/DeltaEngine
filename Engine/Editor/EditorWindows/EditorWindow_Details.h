#pragma once

#include "EngineIncludes.h"

#include "EditorWindows/EditorWindow.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/DVectorProperty.h"
#include "Runtime/Core/DObject.h"
#include "UIComponents/PropertyWidgets/Vec3Field.h"
#include "UIComponents/PropertyWidgets/ScalarField.h"
#include "UIComponents/PropertyWidgets/ColorField.h"
#include "UIComponents/PropertyWidgets/StringField.h"
#include "UIComponents/PropertyWidgets/ReferenceField.h"

DELTA_ENGINE_NS_BEGIN

class GameObject;
class SceneComponent;
class DComponent;

class EditorWindow_Details : public EditorWindow
{
public:
    EditorWindow_Details();
    ~EditorWindow_Details();

    /// Property inspector for the current selection (asset, GameObject, or component).
    void Render() override;

    /// ImGui window title.
    const char* m_title = "Details";
    /// Optional open flag for ImGui::Begin.
    bool* m_open = nullptr;

private:
    void RenderAssetDetails(const AssetId& assetId);
    void RenderGameObjectDetails(GameObject* gameObject);
    void RenderComponentDetails(DComponent* component);
    void RenderSceneComponentTransform(SceneComponent* sceneComponent);

    void DrawPropertyEditor(DObject* instance, DClass* dclass, int depth = 0);
    void DrawReadOnlyProperty(const std::string& label, const std::string& value) const;
    void DrawVectorElements(const DVectorPropertyBase* vectorProp, void* instance, int depth);

    bool DrawIntProperty(DObject* instance, DProperty* prop);
    bool DrawFloatProperty(DObject* instance, DProperty* prop);
    bool DrawDoubleProperty(DObject* instance, DProperty* prop);
    bool DrawBoolProperty(DObject* instance, DProperty* prop);
    bool DrawStringProperty(DObject* instance, DProperty* prop);
    bool DrawWStringProperty(DObject* instance, DProperty* prop);
    bool DrawVector3Property(DObject* instance, DProperty* prop);
    bool DrawQuaternionProperty(DObject* instance, DProperty* prop);
    bool DrawFloat4Property(DObject* instance, DProperty* prop);
    bool DrawFloat4x4Property(DObject* instance, DProperty* prop);
    bool DrawObjectPtrProperty(DObject* instance, DProperty* prop, int depth);
    bool DrawSharedObjectPtrProperty(DObject* instance, DProperty* prop, int depth);
    bool DrawBulkDataProperty(DObject* instance, DProperty* prop);
    bool DrawVectorProperty(DObject* instance, DProperty* prop, int depth);

    Vec3Field      m_vec3Field;
    ScalarField    m_scalarField;
    ColorField     m_colorField;
    StringField    m_stringField;
    ReferenceField m_refField;
};

DELTA_ENGINE_NS_END
