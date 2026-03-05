#include "Editor/EditorWindows/EditorWindow_Details.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/Style/EditorTheme.h"
#include "Editor/UIComponents/PropertyWidgets/PropertyWidgetUtil.h"
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
    const auto& components  = selectionState->GetSelectedComponents();

    if (gameObjects.empty() && components.empty())
    {
        ImGui::TextDisabled("Select a GameObject or component");
        ImGui::End();
        return;
    }

    for (const auto& gameObject : gameObjects)
    {
        if (gameObject)
            RenderGameObjectDetails(gameObject);
    }

    for (const auto& component : components)
    {
        if (component)
            RenderComponentDetails(component);
    }

    ImGui::End();
}

void EditorWindow_Details::RenderGameObjectDetails(GameObject* gameObject)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;

    // Object name — bold + bright
    if (theme->GetBoldFont()) ImGui::PushFont(theme->GetBoldFont());
    ImGui::PushStyleColor(ImGuiCol_Text, c.TBright);
    ImGui::TextUnformatted(gameObject->GetName().c_str());
    ImGui::PopStyleColor();
    if (theme->GetBoldFont()) ImGui::PopFont();

    // Class name — dim
    ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    DClass* dc = gameObject->GetClass();
    ImGui::TextUnformatted(dc ? dc->GetName().c_str() : "GameObject");
    ImGui::PopStyleColor();

    auto rootSceneComponent = gameObject->GetRootSceneComponent();
    if (rootSceneComponent)
    {
        ImGui::Separator();
        ImGui::TextUnformatted("Root Scene Component");
        RenderSceneComponentTransform(rootSceneComponent);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Properties");
    if (dc)
        DrawPropertyEditor(gameObject, dc);
    else
        ImGui::TextDisabled("No reflection data");

    ImGui::Separator();
}

void EditorWindow_Details::RenderComponentDetails(DComponent* component)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;

    // Component name — bold + bright
    if (theme->GetBoldFont()) ImGui::PushFont(theme->GetBoldFont());
    ImGui::PushStyleColor(ImGuiCol_Text, c.TBright);
    ImGui::TextUnformatted(component->GetName().c_str());
    ImGui::PopStyleColor();
    if (theme->GetBoldFont()) ImGui::PopFont();

    // Class name — dim
    ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    DClass* dc = component->GetClass();
    ImGui::TextUnformatted(dc ? dc->GetName().c_str() : "Component");
    ImGui::PopStyleColor();

    auto sceneComponent = dynamic_cast<SceneComponent*>(component);
    if (sceneComponent)
        RenderSceneComponentTransform(sceneComponent);

    ImGui::Separator();
    ImGui::TextUnformatted("Properties");
    if (dc)
        DrawPropertyEditor(component, dc);
    else
        ImGui::TextDisabled("No reflection data");

    ImGui::Separator();
}

void EditorWindow_Details::RenderSceneComponentTransform(SceneComponent* sceneComponent)
{
    if (!sceneComponent)
        return;

    Vector3 pos   = sceneComponent->GetLocalPosition();
    Vector3 euler = sceneComponent->GetLocalRotationEulerAngles();
    Vector3 scale = sceneComponent->GetLocalScale();

    bool changed = false;
    changed |= m_vec3Field.Draw("Position", &pos.x,   0.1f);
    changed |= m_vec3Field.Draw("Rotation", &euler.x, 1.0f);
    changed |= m_vec3Field.Draw("Scale",    &scale.x, 0.01f);

    if (changed)
    {
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
                ImGui::Text("%s: %s", prop->GetName().c_str(),
                    prop->ToString(prop->GetValue(instance)).c_str());
                break;
            }

            ImGui::PopID();
        }
    }
}

bool EditorWindow_Details::DrawIntProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;

    void* addr = prop->GetValue(instance);
    int*  val  = static_cast<int*>(addr);

    float availW = BeginPropertyRow(prop->GetName().c_str(), c);
    ImGui::SetNextItemWidth(availW);
    bool changed = ImGui::DragInt("##v", val);
    EndPropertyRow();

    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawFloatProperty(DObject* instance, DProperty* prop)
{
    void*  addr = prop->GetValue(instance);
    float* val  = static_cast<float*>(addr);

    bool changed = m_scalarField.Draw(prop->GetName().c_str(), val, 0.1f);
    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawDoubleProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;

    void*   addr = prop->GetValue(instance);
    double* val  = static_cast<double*>(addr);

    float availW = BeginPropertyRow(prop->GetName().c_str(), c);
    ImGui::SetNextItemWidth(availW);
    bool changed = ImGui::InputDouble("##v", val, 0.1, 1.0, "%.6f");
    EndPropertyRow();

    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawBoolProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;

    void* addr = prop->GetValue(instance);
    bool* val  = static_cast<bool*>(addr);

    BeginPropertyRow(prop->GetName().c_str(), c);
    bool changed = ImGui::Checkbox("##v", val);
    EndPropertyRow();

    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawStringProperty(DObject* instance, DProperty* prop)
{
    const std::string& current = *static_cast<const std::string*>(prop->GetValue(instance));
    char buf[1024];
    size_t len = (std::min)(current.size(), sizeof(buf) - 1);
    memcpy(buf, current.c_str(), len);
    buf[len] = '\0';
    buf[sizeof(buf) - 1] = '\0';

    if (m_stringField.Draw(prop->GetName().c_str(), buf, sizeof(buf)))
    {
        std::string newVal(buf);
        prop->SetValue(instance, &newVal);
        return true;
    }
    return false;
}

bool EditorWindow_Details::DrawWStringProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;

    const std::wstring& ws  = *static_cast<const std::wstring*>(prop->GetValue(instance));
    std::string         utf8(ws.begin(), ws.end());

    BeginPropertyRow(prop->GetName().c_str(), c);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::TextUnformatted(utf8.c_str());
    ImGui::PopStyleColor();
    EndPropertyRow();

    return false;
}

bool EditorWindow_Details::DrawVector3Property(DObject* instance, DProperty* prop)
{
    void*    addr = prop->GetValue(instance);
    Vector3* val  = static_cast<Vector3*>(addr);

    bool changed = m_vec3Field.Draw(prop->GetName().c_str(), &val->x, 0.1f);
    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawQuaternionProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;

    void*       addr = prop->GetValue(instance);
    Quaternion* val  = static_cast<Quaternion*>(addr);

    float availW = BeginPropertyRow(prop->GetName().c_str(), c);
    ImGui::SetNextItemWidth(availW);
    bool changed = ImGui::DragFloat4("##v", &val->x, 0.01f);
    EndPropertyRow();

    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawFloat4Property(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;

    void*               addr = prop->GetValue(instance);
    DirectX::XMFLOAT4* val  = static_cast<DirectX::XMFLOAT4*>(addr);

    float availW = BeginPropertyRow(prop->GetName().c_str(), c);
    ImGui::SetNextItemWidth(availW);
    bool changed = ImGui::DragFloat4("##v", &val->x, 0.01f);
    EndPropertyRow();

    if (changed)
    {
        DirectX::XMVECTOR v = DirectX::XMLoadFloat4(val);
        prop->SetValue(instance, &v);
    }
    return changed;
}

bool EditorWindow_Details::DrawFloat4x4Property(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto&  c     = theme->colors;

    void*                addr = prop->GetValue(instance);
    DirectX::XMFLOAT4X4* mat = static_cast<DirectX::XMFLOAT4X4*>(addr);
    bool changed = false;

    if (ImGui::TreeNodeEx(prop->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        float availW = BeginPropertyRow("Row 0", c);
        ImGui::SetNextItemWidth(availW);
        changed |= ImGui::DragFloat4("##r0", &mat->_11, 0.01f);
        EndPropertyRow();

        availW = BeginPropertyRow("Row 1", c);
        ImGui::SetNextItemWidth(availW);
        changed |= ImGui::DragFloat4("##r1", &mat->_21, 0.01f);
        EndPropertyRow();

        availW = BeginPropertyRow("Row 2", c);
        ImGui::SetNextItemWidth(availW);
        changed |= ImGui::DragFloat4("##r2", &mat->_31, 0.01f);
        EndPropertyRow();

        availW = BeginPropertyRow("Row 3", c);
        ImGui::SetNextItemWidth(availW);
        changed |= ImGui::DragFloat4("##r3", &mat->_41, 0.01f);
        EndPropertyRow();

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
    DObject*    child = prop->GetObjectPointer(instance);
    const char* label = prop->GetName().c_str();

    if (child == nullptr)
    {
        m_refField.Draw(label, "(null)", true);
        return false;
    }

    constexpr int kMaxDepth = 8;
    if (depth >= kMaxDepth)
    {
        ImGui::Text("%s: %s (max depth)", label, prop->GetType().c_str());
        return false;
    }

    DClass*     childClass = child->GetClass();
    const char* typeName   = childClass ? childClass->GetName().c_str() : "?";
    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen, "%s (%s)", label, typeName))
    {
        DrawPropertyEditor(child, childClass, depth + 1);
        ImGui::TreePop();
    }
    return false;
}

bool EditorWindow_Details::DrawSharedObjectPtrProperty(DObject* instance, DProperty* prop, int depth)
{
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
        "WARNING: SharedObjectPtr will be deprecated in a future version");

    DObject*    child = prop->GetObjectPointer(instance);
    const char* label = prop->GetName().c_str();

    if (child == nullptr)
    {
        m_refField.Draw(label, "(null)", true);
        return false;
    }

    constexpr int kMaxDepth = 8;
    if (depth >= kMaxDepth)
    {
        ImGui::Text("%s: %s (max depth)", label, prop->GetType().c_str());
        return false;
    }

    DClass*     childClass = child->GetClass();
    const char* typeName   = childClass ? childClass->GetName().c_str() : "?";
    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen, "%s (%s)", label, typeName))
    {
        DrawPropertyEditor(child, childClass, depth + 1);
        ImGui::TreePop();
    }
    return false;
}
