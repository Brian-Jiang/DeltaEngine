#include "Editor/EditorWindows/EditorWindow_Details.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/Style/EditorTheme.h"
#include "Editor/UIComponents/PropertyWidgets/PropertyWidgetUtil.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Reflection/DBulkDataProperty.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Utils/StringUtils.h"

#include "SimpleMath.h"
#include <DirectXMath.h>

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <string>

using namespace DeltaEngine;
using namespace DirectX::SimpleMath;

namespace
{
std::string GetPropertyDisplayName(const std::string& propName)
{
    if (propName.empty())
        return {};

    std::string name = propName;
    if (name.size() >= 2 && name[0] == 'm' && name[1] == '_')
        name.erase(0, 2);

    if (name.empty())
        return {};

    std::string result;
    result.reserve(name.size() + 8);
    bool prevUpper = false;
    bool prevLower = false;

    for (size_t i = 0; i < name.size(); ++i)
    {
        const char c = name[i];
        const bool isUpper = std::isupper(static_cast<unsigned char>(c));
        const bool isLower = std::islower(static_cast<unsigned char>(c));

        if (i == 0)
        {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        else if (isUpper)
        {
            if (prevLower || (!prevUpper && std::isdigit(static_cast<unsigned char>(name[i - 1]))))
                result += ' ';
            result += c;
        }
        else
        {
            result += c;
        }

        prevUpper = isUpper;
        prevLower = isLower;
    }

    return result;
}

std::string GetAssetDisplayName(const std::filesystem::path& path)
{
    return path.stem().stem().string();
}
}

EditorWindow_Details::EditorWindow_Details() = default;
EditorWindow_Details::~EditorWindow_Details() = default;

void EditorWindow_Details::Render()
{
    if (!ImGui::Begin(m_title, m_open))
    {
        ImGui::End();
        return;
    }

    if (!g_editorCore || !g_editorCore->GetSelectionState())
    {
        ImGui::TextDisabled("No editor");
        ImGui::End();
        return;
    }

    EditorSelectionState* selectionState = g_editorCore->GetSelectionState();
    const AssetId selectedAssetId = selectionState->GetSelectedAssetId();
    const auto& gameObjects = selectionState->GetSelectedGameObjects();
    const auto& components = selectionState->GetSelectedComponents();

    if (!selectedAssetId.IsNull())
    {
        RenderAssetDetails(selectedAssetId);
        ImGui::End();
        return;
    }

    if (gameObjects.empty() && components.empty())
    {
        ImGui::TextDisabled("Select a GameObject, component, or asset");
        ImGui::End();
        return;
    }

    for (GameObject* gameObject : gameObjects)
    {
        if (gameObject)
            RenderGameObjectDetails(gameObject);
    }

    for (DComponent* component : components)
    {
        if (component)
            RenderComponentDetails(component);
    }

    ImGui::End();
}

void EditorWindow_Details::RenderAssetDetails(const AssetId& assetId)
{
    if (!g_editor)
    {
        ImGui::TextDisabled("No editor");
        return;
    }

    EditorAssetDatabase* assetDatabase = g_editorCore->GetAssetDatabase();
    if (!assetDatabase)
    {
        ImGui::TextDisabled("No asset database");
        return;
    }

    DPrimaryAsset* asset = assetDatabase->GetLoadedAsset(assetId);
    if (!asset)
        asset = assetDatabase->LoadAsset(assetId);

    if (!asset)
    {
        ImGui::TextDisabled("Failed to load asset");
        return;
    }

    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;
    const std::filesystem::path assetPath = assetDatabase->GetAssetPath(assetId);

    if (theme->GetBoldFont())
        ImGui::PushFont(theme->GetBoldFont());
    ImGui::PushStyleColor(ImGuiCol_Text, c.TBright);
    ImGui::TextUnformatted(GetAssetDisplayName(assetPath).c_str());
    ImGui::PopStyleColor();
    if (theme->GetBoldFont())
        ImGui::PopFont();

    ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x);
    ImGui::PushStyleColor(ImGuiCol_Text, asset->IsDirty() ? c.Warn : c.TDim);
    ImGui::TextUnformatted(asset->IsDirty() ? "Dirty" : "Saved");
    ImGui::PopStyleColor();

    ImGui::Separator();
    DrawReadOnlyProperty("Path", assetPath.generic_string());
    DrawReadOnlyProperty("Class", asset->GetHeader().m_className);
    DrawReadOnlyProperty("Asset Id", asset->GetAssetId().ToString());
    DrawReadOnlyProperty("Objects", std::to_string(asset->GetObjects().size()));

    int objectIndex = 0;
    for (DObject* object : asset->GetObjects())
    {
        if (!object)
            continue;

        DClass* dclass = object->GetClass();
        const std::string title = "Object " + std::to_string(objectIndex++) + " - " +
            (dclass ? dclass->GetName() : "Unknown");

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen;
        if (ImGui::CollapsingHeader(title.c_str(), flags))
        {
            if (dclass)
                DrawPropertyEditor(object, dclass);
            else
                ImGui::TextDisabled("No reflection data");
        }
    }
}

void EditorWindow_Details::RenderGameObjectDetails(GameObject* gameObject)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    if (theme->GetBoldFont())
        ImGui::PushFont(theme->GetBoldFont());
    ImGui::PushStyleColor(ImGuiCol_Text, c.TBright);
    ImGui::TextUnformatted(gameObject->GetName().c_str());
    ImGui::PopStyleColor();
    if (theme->GetBoldFont())
        ImGui::PopFont();

    ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    DClass* dc = gameObject->GetClass();
    ImGui::TextUnformatted(dc ? dc->GetName().c_str() : "GameObject");
    ImGui::PopStyleColor();

    if (SceneComponent* rootSceneComponent = gameObject->GetRootSceneComponent())
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
    const auto& c = theme->colors;

    if (theme->GetBoldFont())
        ImGui::PushFont(theme->GetBoldFont());
    ImGui::PushStyleColor(ImGuiCol_Text, c.TBright);
    ImGui::TextUnformatted(component->GetName().c_str());
    ImGui::PopStyleColor();
    if (theme->GetBoldFont())
        ImGui::PopFont();

    ImGui::SameLine(0.f, ImGui::GetStyle().ItemSpacing.x);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    DClass* dc = component->GetClass();
    ImGui::TextUnformatted(dc ? dc->GetName().c_str() : "Component");
    ImGui::PopStyleColor();

    if (SceneComponent* sceneComponent = dynamic_cast<SceneComponent*>(component))
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

    Vector3 pos = sceneComponent->GetLocalPosition();
    Vector3 euler = sceneComponent->GetLocalRotationEulerAngles();
    Vector3 scale = sceneComponent->GetLocalScale();

    bool changed = false;
    changed |= m_vec3Field.Draw("Position", &pos.x, 0.1f);
    changed |= m_vec3Field.Draw("Rotation", &euler.x, 1.0f);
    changed |= m_vec3Field.Draw("Scale", &scale.x, 0.01f);

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
            case EPropertyType::BulkData:
                DrawBulkDataProperty(instance, prop);
                break;
            case EPropertyType::Vector:
                DrawVectorProperty(instance, prop, depth);
                break;
            case EPropertyType::Struct:
                DrawReadOnlyProperty(GetPropertyDisplayName(prop->GetName()),
                    prop->ToString(prop->GetValue(instance)));
                break;
            default:
                DrawReadOnlyProperty(GetPropertyDisplayName(prop->GetName()),
                    prop->ToString(prop->GetValue(instance)));
                break;
            }

            ImGui::PopID();
        }
    }
}

void EditorWindow_Details::DrawReadOnlyProperty(const std::string& label, const std::string& value) const
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    const float availW = BeginPropertyRow(label.c_str(), c);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + availW);
    ImGui::TextUnformatted(value.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopStyleColor();
    EndPropertyRow();
}

void EditorWindow_Details::DrawVectorElements(const DVectorPropertyBase* vectorProp, void* instance, int depth)
{
    if (!vectorProp || !instance)
        return;

    constexpr int kMaxDepth = 8;
    const DProperty* innerProp = vectorProp->GetInnerProperty();
    if (!innerProp)
        return;

    const size_t count = vectorProp->GetSize(instance);
    for (size_t index = 0; index < count; ++index)
    {
        void* elementAddr = vectorProp->GetElementAddress(instance, index);
        if (!elementAddr)
            continue;

        const std::string label = "[" + std::to_string(index) + "]";
        switch (innerProp->GetPropertyType())
        {
        case EPropertyType::Vector:
        {
            const auto* nestedVector = dynamic_cast<const DVectorPropertyBase*>(innerProp);
            if (!nestedVector)
            {
                DrawReadOnlyProperty(label, innerProp->ToString(elementAddr));
                break;
            }

            if (depth >= kMaxDepth)
            {
                DrawReadOnlyProperty(label, innerProp->ToString(elementAddr));
                break;
            }

            if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "%s", label.c_str()))
            {
                DrawVectorElements(nestedVector, elementAddr, depth + 1);
                ImGui::TreePop();
            }
            break;
        }
        case EPropertyType::ObjectPtr:
        case EPropertyType::SharedObjectPtr:
        {
            DObject* child = innerProp->GetObjectPointer(elementAddr);
            if (!child)
            {
                DrawReadOnlyProperty(label, "(null)");
                break;
            }

            DClass* childClass = child->GetClass();
            const std::string nodeLabel = label + " (" + (childClass ? childClass->GetName() : "?") + ")";
            if (depth >= kMaxDepth)
            {
                DrawReadOnlyProperty(label, nodeLabel);
                break;
            }

            if (ImGui::TreeNodeEx(nodeLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawPropertyEditor(child, childClass, depth + 1);
                ImGui::TreePop();
            }
            break;
        }
        default:
            DrawReadOnlyProperty(label, innerProp->ToString(elementAddr));
            break;
        }
    }
}

bool EditorWindow_Details::DrawIntProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    int* val = static_cast<int*>(prop->GetValue(instance));

    const float availW = BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    ImGui::SetNextItemWidth(availW);
    const bool changed = ImGui::DragInt("##v", val);
    EndPropertyRow();

    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawFloatProperty(DObject* instance, DProperty* prop)
{
    float* val = static_cast<float*>(prop->GetValue(instance));

    const bool changed = m_scalarField.Draw(GetPropertyDisplayName(prop->GetName()).c_str(), val, 0.1f);
    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawDoubleProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    double* val = static_cast<double*>(prop->GetValue(instance));

    const float availW = BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    ImGui::SetNextItemWidth(availW);
    const bool changed = ImGui::InputDouble("##v", val, 0.1, 1.0, "%.6f");
    EndPropertyRow();

    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawBoolProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    bool* val = static_cast<bool*>(prop->GetValue(instance));

    BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    const bool changed = ImGui::Checkbox("##v", val);
    EndPropertyRow();

    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawStringProperty(DObject* instance, DProperty* prop)
{
    const std::string& current = *static_cast<const std::string*>(prop->GetValue(instance));
    char buf[1024];
    const size_t len = (std::min)(current.size(), sizeof(buf) - 1);
    memcpy(buf, current.c_str(), len);
    buf[len] = '\0';
    buf[sizeof(buf) - 1] = '\0';

    if (m_stringField.Draw(GetPropertyDisplayName(prop->GetName()).c_str(), buf, sizeof(buf)))
    {
        const std::string newVal(buf);
        prop->SetValue(instance, &newVal);
        return true;
    }
    return false;
}

bool EditorWindow_Details::DrawWStringProperty(DObject* instance, DProperty* prop)
{
    const std::wstring& ws = *static_cast<const std::wstring*>(prop->GetValue(instance));
    DrawReadOnlyProperty(GetPropertyDisplayName(prop->GetName()), StringUtils::WStringToUtf8(ws));
    return false;
}

bool EditorWindow_Details::DrawVector3Property(DObject* instance, DProperty* prop)
{
    Vector3* val = static_cast<Vector3*>(prop->GetValue(instance));

    const bool changed = m_vec3Field.Draw(GetPropertyDisplayName(prop->GetName()).c_str(), &val->x, 0.1f);
    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawQuaternionProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    Quaternion* val = static_cast<Quaternion*>(prop->GetValue(instance));

    const float availW = BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    ImGui::SetNextItemWidth(availW);
    const bool changed = ImGui::DragFloat4("##v", &val->x, 0.01f);
    EndPropertyRow();

    if (changed)
        prop->SetValue(instance, val);
    return changed;
}

bool EditorWindow_Details::DrawFloat4Property(DObject* instance, DProperty* prop)
{
    void* addr = prop->GetValue(instance);

    if (prop->GetMeta("UIType") == "Color")
    {
        float* val = static_cast<float*>(addr);
        const bool changed = m_colorField.Draw(GetPropertyDisplayName(prop->GetName()).c_str(), val, true);
        if (changed)
            prop->SetValue(instance, val);
        return changed;
    }

    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    auto* val = static_cast<DirectX::XMFLOAT4*>(addr);

    const float availW = BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    ImGui::SetNextItemWidth(availW);
    const bool changed = ImGui::DragFloat4("##v", &val->x, 0.01f);
    EndPropertyRow();

    if (changed)
    {
        const DirectX::XMVECTOR vectorValue = DirectX::XMLoadFloat4(val);
        prop->SetValue(instance, &vectorValue);
    }
    return changed;
}

bool EditorWindow_Details::DrawFloat4x4Property(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    auto* mat = static_cast<DirectX::XMFLOAT4X4*>(prop->GetValue(instance));
    bool changed = false;

    const std::string displayName = GetPropertyDisplayName(prop->GetName());
    if (ImGui::TreeNodeEx(displayName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
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
        const DirectX::XMMATRIX matrixValue = DirectX::XMLoadFloat4x4(mat);
        prop->SetValue(instance, &matrixValue);
    }
    return changed;
}

bool EditorWindow_Details::DrawObjectPtrProperty(DObject* instance, DProperty* prop, int depth)
{
    DObject* child = prop->GetObjectPointer(instance);
    const std::string displayName = GetPropertyDisplayName(prop->GetName());
    const char* label = displayName.c_str();

    if (!child)
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
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
        "WARNING: SharedObjectPtr will be deprecated in a future version");

    DObject* child = prop->GetObjectPointer(instance);
    const std::string displayName = GetPropertyDisplayName(prop->GetName());
    const char* label = displayName.c_str();

    if (!child)
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

    DClass* childClass = child->GetClass();
    const char* typeName = childClass ? childClass->GetName().c_str() : "?";
    if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen, "%s (%s)", label, typeName))
    {
        DrawPropertyEditor(child, childClass, depth + 1);
        ImGui::TreePop();
    }
    return false;
}

bool EditorWindow_Details::DrawBulkDataProperty(DObject* instance, DProperty* prop)
{
    const auto* bulk = static_cast<const TBulkData*>(prop->GetValue(instance));
    DrawReadOnlyProperty(GetPropertyDisplayName(prop->GetName()),
        "BulkData(id=" + std::to_string(bulk->m_bulkId) + ", size=" + std::to_string(bulk->m_size) + " bytes)");
    return false;
}

bool EditorWindow_Details::DrawVectorProperty(DObject* instance, DProperty* prop, int depth)
{
    const auto* vectorProp = dynamic_cast<const DVectorPropertyBase*>(prop);
    if (!vectorProp)
    {
        DrawReadOnlyProperty(GetPropertyDisplayName(prop->GetName()), prop->ToString(prop->GetValue(instance)));
        return false;
    }

    const std::string displayName = GetPropertyDisplayName(prop->GetName());
    const size_t count = vectorProp->GetSize(instance);
    if (ImGui::TreeNodeEx(displayName.c_str(), ImGuiTreeNodeFlags_DefaultOpen,
        "%s [%zu]", displayName.c_str(), count))
    {
        DrawVectorElements(vectorProp, instance, depth + 1);
        ImGui::TreePop();
    }
    return false;
}
