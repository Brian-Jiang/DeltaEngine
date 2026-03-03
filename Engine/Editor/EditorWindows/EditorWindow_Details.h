#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "EditorWindows/EditorWindow.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Core/DObject.h"

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

    void DrawPropertyEditor(DObject* instance, DClass* dclass, int depth = 0);
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
};

DELTA_ENGINE_NS_END
