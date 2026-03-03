#include "Editor/EditorWindows/EditorWindow_Details.h"

#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include "SimpleMath.h"
#include <DirectXMath.h>
#include <algorithm>

#include "imgui.h"

using namespace DeltaEngine;
using namespace DirectX::SimpleMath;

EditorWindow_Details::EditorWindow_Details()
{
}

EditorWindow_Details::~EditorWindow_Details()
{
}

void EditorWindow_Details::Render()
{
    if (!ImGui::Begin(m_title, m_open))
    {
        ImGui::End();
        return;
    }

    if (!g_editor || !g_editor->GetSelectionState())
    {
        ImGui::TextDisabled("No editor");
        ImGui::End();
        return;
    }

    auto selectionState = g_editor->GetSelectionState();
    const auto& gameObjects = selectionState->GetSelectedGameObjects();
    const auto& components = selectionState->GetSelectedComponents();

    if (gameObjects.empty() && components.empty())
    {
        ImGui::TextDisabled("Select a GameObject or component");
        ImGui::End();
        return;
    }

    for (const auto& gameObject : gameObjects)
    {
        if (gameObject)
        {
            RenderGameObjectDetails(gameObject);
        }
    }

    for (const auto& component : components)
    {
        if (component)
        {
            RenderComponentDetails(component);
        }
    }

    ImGui::End();
}

void EditorWindow_Details::RenderGameObjectDetails(GameObject* gameObject)
{
    ImGui::TextUnformatted("GameObject");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", gameObject->GetName().c_str());

    auto rootSceneComponent = gameObject->GetRootSceneComponent();
    if (rootSceneComponent)
    {
        ImGui::Separator();
        ImGui::TextUnformatted("Root Scene Component");
        RenderSceneComponentTransform(rootSceneComponent);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Properties");
    DClass* dclass = gameObject->GetClass();
    if (dclass)
        DrawPropertyEditor(gameObject, dclass);
    else
        ImGui::TextDisabled("No reflection data");

    ImGui::Separator();
}

void EditorWindow_Details::RenderComponentDetails(DComponent* component)
{
    ImGui::TextUnformatted("Component");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", component->GetName().c_str());

    auto sceneComponent = dynamic_cast<SceneComponent*>(component);
    if (sceneComponent)
    {
        RenderSceneComponentTransform(sceneComponent);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Properties");
    DClass* dclass = component->GetClass();
    if (dclass)
        DrawPropertyEditor(component, dclass);
    else
        ImGui::TextDisabled("No reflection data");

    ImGui::Separator();
}

void EditorWindow_Details::RenderSceneComponentTransform(SceneComponent* sceneComponent)
{
    if (!sceneComponent)
        return;

    Vector3 pos = sceneComponent->GetLocalPosition();
    Vector3 euler = sceneComponent->GetLocalRotationEulerAngles();
    Vector3 scale = sceneComponent->GetLocalScale();
    //const float radToDeg = 180.0f / DirectX::XM_PI;
    //euler *= radToDeg;

    bool changed = false;
    changed |= ImGui::DragFloat3("Position", &pos.x, 0.1f);
    changed |= ImGui::DragFloat3("Rotation", &euler.x, 1.0f);
    changed |= ImGui::DragFloat3("Scale", &scale.x, 0.01f);

    if (changed)
    {
        const float degToRad = DirectX::XM_PI / 180.0f;
        sceneComponent->SetLocalPosition(pos);
        sceneComponent->SetLocalRotation(euler);
        sceneComponent->SetLocalScale(scale);
    }
}

void EditorWindow_Details::DrawPropertyEditor(DObject* instance, DClass* dclass, int depth)
{
    if (!instance || !dclass)
        return;

    constexpr int kMaxDepth = 8;
    if (depth >= kMaxDepth)
        return;

    for (DStruct* s = dclass; s; s = s->GetSuper())
    {
        for (DProperty* prop = s->GetOwnProperties(); prop; prop = prop->GetNext())
    {
        ImGui::PushID(prop->GetName().c_str());

        const char* label = prop->GetName().c_str();
        switch (prop->GetPropertyType())
        {
        case EPropertyType::Int:
            DrawIntProperty(instance, prop);
            break;
        case EPropertyType::Float:
            DrawFloatProperty(instance, prop);
            break;
        case EPropertyType::Double:
            DrawDoubleProperty(instance, prop);
            break;
        case EPropertyType::Bool:
            DrawBoolProperty(instance, prop);
            break;
        case EPropertyType::String:
            DrawStringProperty(instance, prop);
            break;
        case EPropertyType::WString:
            DrawWStringProperty(instance, prop);
            break;
        case EPropertyType::Vector3:
            DrawVector3Property(instance, prop);
            break;
        case EPropertyType::Quaternion:
            DrawQuaternionProperty(instance, prop);
            break;
        case EPropertyType::Float4:
            DrawFloat4Property(instance, prop);
            break;
        case EPropertyType::Float4x4:
            DrawFloat4x4Property(instance, prop);
            break;
        case EPropertyType::ObjectPtr:
            DrawObjectPtrProperty(instance, prop, depth);
            break;
        case EPropertyType::SharedObjectPtr:
            DrawSharedObjectPtrProperty(instance, prop, depth);
            break;
        default:
            ImGui::Text("%s: %s", label, prop->ToString(prop->GetValue(instance)).c_str());
            break;
        }

        ImGui::PopID();
    }
    }
}

bool EditorWindow_Details::DrawIntProperty(DObject* instance, DProperty* prop)
{
    void* addr = prop->GetValue(instance);
    int* val = static_cast<int*>(addr);
    if (ImGui::DragInt(prop->GetName().c_str(), val))
    {
        prop->SetValue(instance, val);
        return true;
    }
    return false;
}

bool EditorWindow_Details::DrawFloatProperty(DObject* instance, DProperty* prop)
{
    void* addr = prop->GetValue(instance);
    float* val = static_cast<float*>(addr);
    if (ImGui::DragFloat(prop->GetName().c_str(), val, 0.1f))
    {
        prop->SetValue(instance, val);
        return true;
    }
    return false;
}

bool EditorWindow_Details::DrawDoubleProperty(DObject* instance, DProperty* prop)
{
    void* addr = prop->GetValue(instance);
    double* val = static_cast<double*>(addr);
    if (ImGui::InputDouble(prop->GetName().c_str(), val, 0.1, 1.0, "%.6f"))
    {
        prop->SetValue(instance, val);
        return true;
    }
    return false;
}

bool EditorWindow_Details::DrawBoolProperty(DObject* instance, DProperty* prop)
{
    void* addr = prop->GetValue(instance);
    bool* val = static_cast<bool*>(addr);
    if (ImGui::Checkbox(prop->GetName().c_str(), val))
    {
        prop->SetValue(instance, val);
        return true;
    }
    return false;
}

bool EditorWindow_Details::DrawStringProperty(DObject* instance, DProperty* prop)
{
    const std::string& current = *static_cast<const std::string*>(prop->GetValue(instance));
    char buf[1024];
    size_t len = (std::min)(current.size(), sizeof(buf) - 1);
    memcpy(buf, current.c_str(), len);
    buf[len] = '\0';
    buf[sizeof(buf) - 1] = '\0';
    if (ImGui::InputText(prop->GetName().c_str(), buf, sizeof(buf)))
    {
        std::string newVal(buf);
        prop->SetValue(instance, &newVal);
        return true;
    }
    return false;
}

bool EditorWindow_Details::DrawWStringProperty(DObject* instance, DProperty* prop)
{
    const std::wstring& ws = *static_cast<const std::wstring*>(prop->GetValue(instance));
    std::string utf8(ws.begin(), ws.end());
    ImGui::Text("%s: %s", prop->GetName().c_str(), utf8.c_str());
    return false;
}

bool EditorWindow_Details::DrawVector3Property(DObject* instance, DProperty* prop)
{
    void* addr = prop->GetValue(instance);
    Vector3* val = static_cast<Vector3*>(addr);
    if (ImGui::DragFloat3(prop->GetName().c_str(), &val->x, 0.1f))
    {
        prop->SetValue(instance, val);
        return true;
    }
    return false;
}

bool EditorWindow_Details::DrawQuaternionProperty(DObject* instance, DProperty* prop)
{
    void* addr = prop->GetValue(instance);
    Quaternion* val = static_cast<Quaternion*>(addr);
    if (ImGui::DragFloat4(prop->GetName().c_str(), &val->x, 0.01f))
    {
        prop->SetValue(instance, val);
        return true;
    }
    return false;
}

bool EditorWindow_Details::DrawFloat4Property(DObject* instance, DProperty* prop)
{
    void* addr = prop->GetValue(instance);
    DirectX::XMFLOAT4* val = static_cast<DirectX::XMFLOAT4*>(addr);
    if (ImGui::DragFloat4(prop->GetName().c_str(), &val->x, 0.01f))
    {
        DirectX::XMVECTOR v = DirectX::XMLoadFloat4(val);
        prop->SetValue(instance, &v);
        return true;
    }
    return false;
}

bool EditorWindow_Details::DrawFloat4x4Property(DObject* instance, DProperty* prop)
{
    void* addr = prop->GetValue(instance);
    DirectX::XMFLOAT4X4* mat = static_cast<DirectX::XMFLOAT4X4*>(addr);
    bool changed = false;
    if (ImGui::TreeNodeEx(prop->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        changed |= ImGui::DragFloat4("Row 0", &mat->_11, 0.01f);
        changed |= ImGui::DragFloat4("Row 1", &mat->_21, 0.01f);
        changed |= ImGui::DragFloat4("Row 2", &mat->_31, 0.01f);
        changed |= ImGui::DragFloat4("Row 3", &mat->_41, 0.01f);
        ImGui::TreePop();
    }
    if (changed)
    {
        DirectX::XMMATRIX m = DirectX::XMLoadFloat4x4(mat);
        prop->SetValue(instance, &m);
    }
    return changed;
}

bool EditorWindow_Details::DrawObjectPtrProperty(DObject* instance, DProperty* prop, int depth)
{
    DObject* child = prop->GetObjectPointer(instance);
    const char* label = prop->GetName().c_str();

    if (child == nullptr)
    {
        ImGui::Text("%s: (null)", label);
        return false;
    }

    constexpr int kMaxDepth = 8;
    if (depth >= kMaxDepth)
    {
        ImGui::Text("%s: %s (max depth)", label, prop->GetType().c_str());
        return false;
    }

    DClass* childClass = child->GetClass();
    const char* typeName = childClass ? childClass->GetName().c_str() : "?";
    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen, "%s (%s)", label, typeName))
    {
        DrawPropertyEditor(child, childClass, depth + 1);
        ImGui::TreePop();
    }
    return false;
}

bool EditorWindow_Details::DrawSharedObjectPtrProperty(DObject* instance, DProperty* prop, int depth)
{
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "⚠ SharedObjectPtr will be deprecated in a future version");

    DObject* child = prop->GetObjectPointer(instance);
    const char* label = prop->GetName().c_str();

    if (child == nullptr)
    {
        ImGui::Text("%s: (null)", label);
        return false;
    }

    constexpr int kMaxDepth = 8;
    if (depth >= kMaxDepth)
    {
        ImGui::Text("%s: %s (max depth)", label, prop->GetType().c_str());
        return false;
    }

    DClass* childClass = child->GetClass();
    const char* typeName = childClass ? childClass->GetName().c_str() : "?";
    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen, "%s (%s)", label, typeName))
    {
        DrawPropertyEditor(child, childClass, depth + 1);
        ImGui::TreePop();
    }
    return false;
}
