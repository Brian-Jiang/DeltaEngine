#include "Editor/EditorWindows/EditorWindow_Details.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/Style/EditorTheme.h"
#include "Editor/UIComponents/PropertyWidgets/PropertyWidgetUtil.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/IO/IOManager.h"
#include "Runtime/Reflection/DBulkDataProperty.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DFunction.h"
#include "Runtime/Reflection/DStruct.h"
#include "Runtime/Reflection/DProperty.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Utils/StringUtils.h"

#include "SimpleMath.h"
#include <DirectXMath.h>

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

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

bool IsUndoablePropertyType(EPropertyType type)
{
    switch (type)
    {
    case EPropertyType::Float:
    case EPropertyType::Int:
    case EPropertyType::Bool:
    case EPropertyType::Double:
    case EPropertyType::String:
    case EPropertyType::Vector3:
    case EPropertyType::Quaternion:
    case EPropertyType::Float4:
    case EPropertyType::Float4x4:
    case EPropertyType::Struct:
        return true;
    default:
        return false;
    }
}

void LivePreviewWrite(DObject* obj, DProperty* prop)
{
    void* addr = prop->GetValue(obj);
    switch (prop->GetPropertyType())
    {
    case EPropertyType::Float4:
    {
        DirectX::XMVECTOR v = DirectX::XMLoadFloat4(static_cast<DirectX::XMFLOAT4*>(addr));
        EditorCommandContext::ApplyReflectedWrite(obj, prop, &v);
        return;
    }
    case EPropertyType::Float4x4:
    {
        DirectX::XMMATRIX m = DirectX::XMLoadFloat4x4(static_cast<DirectX::XMFLOAT4X4*>(addr));
        EditorCommandContext::ApplyReflectedWrite(obj, prop, &m);
        return;
    }
    case EPropertyType::Struct:
        EditorCommandContext::ApplyReflectedWrite(obj, prop, prop->GetValue(obj));
        return;
    default:
        EditorCommandContext::ApplyReflectedWrite(obj, prop, addr);
    }
}
}

EditorWindow_Details::EditorWindow_Details()
{
    m_title = "Details";
}

EditorWindow_Details::~EditorWindow_Details() = default;

void EditorWindow_Details::Render(bool& open)
{
    if (!ImGui::Begin(GetImGuiTitle(), &open))
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

    enum class Category { None, GameObject, Asset, Folder, Component };
    Category category = Category::None;

    if (selectionState->HasComponentSelection())
        category = Category::Component;
    else if (selectionState->HasGameObjectSelection())
        category = Category::GameObject;
    else if (selectionState->HasFolderSelection())
        category = Category::Folder;
    else if (selectionState->HasAssetSelection())
        category = Category::Asset;

    if (category == Category::None)
    {
        ImGui::TextDisabled("Select a GameObject, component, folder, or asset");
        ImGui::End();
        return;
    }

    auto resolveObject = [&](ObjectId id) -> DObject*
    {
        DPrimaryAsset* asset = g_editorCore->GetActiveSceneAsset();
        return asset ? asset->FindObject(id) : nullptr;
    };

    if (category == Category::GameObject)
    {
        const auto& ids = selectionState->GetSelectedGameObjects();
        if (ids.size() > 1)
        {
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y * 0.5f - ImGui::GetTextLineHeight() * 0.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
            const float textW = ImGui::CalcTextSize("Multiple selection not supported").x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - textW) * 0.5f);
            ImGui::TextUnformatted("Multiple selection not supported");
            ImGui::PopStyleVar();
            ImGui::End();
            return;
        }
        DObject* obj = resolveObject(ids[0]);
        if (GameObject* go = dynamic_cast<GameObject*>(obj))
            RenderGameObjectDetails(go);
        else
            ImGui::TextDisabled("Failed to resolve GameObject");
    }
    else if (category == Category::Asset)
    {
        const auto& ids = selectionState->GetSelectedAssets();
        if (ids.size() > 1)
        {
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y * 0.5f - ImGui::GetTextLineHeight() * 0.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
            const float textW = ImGui::CalcTextSize("Multiple selection not supported").x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - textW) * 0.5f);
            ImGui::TextUnformatted("Multiple selection not supported");
            ImGui::PopStyleVar();
            ImGui::End();
            return;
        }
        RenderAssetDetails(ids[0]);
    }
    else if (category == Category::Folder)
    {
        RenderFolderDetails(selectionState->GetSelectedFolder());
    }
    else if (category == Category::Component)
    {
        const auto& ids = selectionState->GetSelectedComponents();
        if (ids.size() > 1)
        {
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + avail.y * 0.5f - ImGui::GetTextLineHeight() * 0.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.6f);
            const float textW = ImGui::CalcTextSize("Multiple selection not supported").x;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x - textW) * 0.5f);
            ImGui::TextUnformatted("Multiple selection not supported");
            ImGui::PopStyleVar();
            ImGui::End();
            return;
        }
        DObject* obj = resolveObject(ids[0]);
        if (DComponent* comp = dynamic_cast<DComponent*>(obj))
            RenderComponentDetails(comp);
        else
            ImGui::TextDisabled("Failed to resolve component");
    }

    ImGui::End();
}

void EditorWindow_Details::RenderFolderDetails(const std::string& folderRelPath)
{
    if (!g_editor)
    {
        ImGui::TextDisabled("No editor");
        return;
    }

    const std::filesystem::path folderPath =
        std::filesystem::path(IOManager::GetEngineImportedAssetsFolder()) / folderRelPath;
    if (!std::filesystem::exists(folderPath))
    {
        ImGui::TextDisabled("Folder does not exist");
        return;
    }

    std::size_t folderCount = 0;
    std::size_t assetCount  = 0;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(folderPath, ec))
    {
        if (entry.is_directory())
        {
            ++folderCount;
            continue;
        }

        const auto path = entry.path();
        if (entry.is_regular_file() && path.extension() == ".json" && path.stem().extension() == ".dasset")
            ++assetCount;
    }

    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;
    if (theme->GetBoldFont())
        ImGui::PushFont(theme->GetBoldFont());
    ImGui::PushStyleColor(ImGuiCol_Text, c.TBright);
    ImGui::TextUnformatted(folderPath.filename().string().c_str());
    ImGui::PopStyleColor();
    if (theme->GetBoldFont())
        ImGui::PopFont();

    ImGui::Separator();
    DrawReadOnlyProperty("Path", folderRelPath);
    DrawReadOnlyProperty("Folders", std::to_string(folderCount));
    DrawReadOnlyProperty("Assets", std::to_string(assetCount));
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

    if (m_transformEditing && m_transformEditTarget != sceneComponent)
    {
        m_transformEditing = false;
        m_transformEditTarget = nullptr;
        m_transformEditBefore = {};
    }

    Vector3 pos = sceneComponent->GetLocalPosition();
    Vector3 euler = sceneComponent->GetLocalRotationEulerAngles();
    Vector3 scale = sceneComponent->GetLocalScale();

    WidgetEditEvent combined;
    combined.Merge(m_vec3Field.Draw("Position", &pos.x, 0.1f));
    combined.Merge(m_vec3Field.Draw("Rotation", &euler.x, 1.0f));
    combined.Merge(m_vec3Field.Draw("Scale", &scale.x, 0.01f));

    if (combined.editBegan && !m_transformEditing)
    {
        m_transformEditing = true;
        m_transformEditTarget = sceneComponent;
        DClass* dc = sceneComponent->GetClass();
        DProperty* ltProp = dc ? dc->FindPropertyByName("m_localTransform") : nullptr;
        if (ltProp)
            m_transformEditBefore = PropertyToJson(sceneComponent, ltProp);
    }

    if (combined.valueChanged)
    {
        sceneComponent->SetLocalPosition(pos);
        sceneComponent->SetLocalRotation(euler);
        sceneComponent->SetLocalScale(scale);
    }

    if (combined.editEnded && m_transformEditing)
    {
        DClass* dc = sceneComponent->GetClass();
        DProperty* ltProp = dc ? dc->FindPropertyByName("m_localTransform") : nullptr;
        if (ltProp)
        {
            nlohmann::json valueAfter = PropertyToJson(sceneComponent, ltProp);
            if (m_transformEditBefore != valueAfter)
            {
                auto [assetId, objectId] = g_editorCore->GetIdsForObject(sceneComponent);
                auto cmd = std::make_unique<EditorCommand_SetProperty>(
                    assetId, objectId,
                    std::string("m_localTransform"),
                    std::move(m_transformEditBefore),
                    std::move(valueAfter));
                EditorCommandContext ctx{ *g_editorCore };
                g_editorCore->GetCommandManager().Execute(std::move(cmd), ctx);
            }
        }
        m_transformEditing = false;
        m_transformEditTarget = nullptr;
        m_transformEditBefore = {};
    }
}

void EditorWindow_Details::DrawPropertyEditor(DObject* instance, DClass* dclass, int depth)
{
    if (!instance || !dclass)
        return;

    constexpr int kMaxDepth = 8;
    if (depth >= kMaxDepth)
        return;

    if (depth == 0 && m_activeEditProp && m_activeEditObject != instance)
    {
        m_activeEditProp   = nullptr;
        m_activeEditBefore = {};
        m_activeEditObject = nullptr;
    }

    std::vector<std::pair<std::string, std::vector<DProperty*>>> buckets;
    auto findOrAddBucket = [&buckets](const std::string& category) -> std::vector<DProperty*>& {
        for (auto& [name, props] : buckets)
        {
            if (name == category)
                return props;
        }
        buckets.emplace_back(category, std::vector<DProperty*>{});
        return buckets.back().second;
    };

    for (DStruct* s = dclass; s; s = s->GetSuper())
    {
        for (DProperty* prop = s->GetOwnProperties(); prop; prop = prop->GetNext())
        {
            if (prop->IsHiddenInDetails())
                continue;

            const std::string category = prop->GetMeta("Category");
            findOrAddBucket(category).push_back(prop);
        }
    }

    for (auto& [category, props] : buckets)
    {
        if (category.empty())
        {
            for (DProperty* prop : props)
                RenderSingleProperty(instance, prop, depth);
        }
        else
        {
            ImGui::PushID(category.c_str());
            const bool open = ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen);
            if (open)
            {
                for (DProperty* prop : props)
                    RenderSingleProperty(instance, prop, depth);
            }
            ImGui::PopID();
        }
    }

    if (depth == 0)
        DrawFunctionButtons(instance, dclass);
}

void EditorWindow_Details::RenderSingleProperty(DObject* instance, DProperty* prop, int depth)
{
    ImGui::PushID(prop->GetName().c_str());

    const bool undoable = IsUndoablePropertyType(prop->GetPropertyType());

    nlohmann::json preSnapshot;
    if (undoable && !m_activeEditProp)
        preSnapshot = PropertyToJson(instance, prop);

    WidgetEditEvent evt;

    switch (prop->GetPropertyType())
    {
    case EPropertyType::Int:
        evt = DrawIntProperty(instance, prop);
        break;
    case EPropertyType::Float:
        evt = DrawFloatProperty(instance, prop);
        break;
    case EPropertyType::Double:
        evt = DrawDoubleProperty(instance, prop);
        break;
    case EPropertyType::Bool:
        evt = DrawBoolProperty(instance, prop);
        break;
    case EPropertyType::String:
        evt = DrawStringProperty(instance, prop);
        break;
    case EPropertyType::WString:
        DrawWStringProperty(instance, prop);
        break;
    case EPropertyType::Vector3:
        evt = DrawVector3Property(instance, prop);
        break;
    case EPropertyType::Quaternion:
        evt = DrawQuaternionProperty(instance, prop);
        break;
    case EPropertyType::Float4:
        evt = DrawFloat4Property(instance, prop);
        break;
    case EPropertyType::Float4x4:
        evt = DrawFloat4x4Property(instance, prop);
        break;
    case EPropertyType::ObjectPtr:
        DrawObjectPtrProperty(instance, prop, depth);
        break;
    case EPropertyType::BulkData:
        DrawBulkDataProperty(instance, prop);
        break;
    case EPropertyType::Vector:
        DrawVectorProperty(instance, prop, depth);
        break;
    case EPropertyType::Struct:
        evt = DrawStructPropertyEditor(instance, static_cast<DStructProperty*>(prop));
        break;
    default:
        DrawReadOnlyProperty(GetPropertyDisplayName(prop->GetName()),
            prop->ToString(prop->GetValue(instance)));
        break;
    }

    if (undoable)
    {
        if (evt.editBegan && !m_activeEditProp)
        {
            m_activeEditProp   = prop;
            m_activeEditObject = instance;
            m_activeEditBefore = std::move(preSnapshot);
        }

        if (evt.valueChanged)
            LivePreviewWrite(instance, prop);

        if (evt.editEnded && m_activeEditProp == prop)
        {
            nlohmann::json valueAfter = PropertyToJson(instance, prop);
            if (m_activeEditBefore != valueAfter)
            {
                auto [assetId, objectId] = g_editorCore->GetIdsForObject(instance);
                auto cmd = std::make_unique<EditorCommand_SetProperty>(
                    assetId, objectId,
                    std::string(prop->GetName()),
                    std::move(m_activeEditBefore),
                    std::move(valueAfter));
                EditorCommandContext ctx{ *g_editorCore };
                g_editorCore->GetCommandManager().Execute(std::move(cmd), ctx);
            }
            m_activeEditProp   = nullptr;
            m_activeEditBefore = {};
            m_activeEditObject = nullptr;
        }
    }

    ImGui::PopID();
}

void EditorWindow_Details::DrawFunctionButtons(DObject* instance, DClass* dclass)
{
    if (!instance || !dclass)
        return;

    std::vector<DFunction*> buttons;
    std::unordered_set<std::string> seen;
    for (DStruct* s = dclass; s; s = s->GetSuper())
    {
        DClass* c = dynamic_cast<DClass*>(s);
        if (!c)
            continue;
        for (const auto& [name, fn] : c->GetFunctions())
        {
            if (!fn || !fn->HasMeta("ShowAsButton"))
                continue;
            if (fn->GetNumParams() != 0)
                continue;
            if (!seen.insert(name).second)
                continue;
            buttons.push_back(fn);
        }
    }

    if (buttons.empty())
        return;

    ImGui::Separator();
    if (!ImGui::CollapsingHeader("Actions", ImGuiTreeNodeFlags_DefaultOpen))
        return;

    for (DFunction* fn : buttons)
    {
        ImGui::PushID(fn);
        if (ImGui::Button(fn->GetName().c_str()))
        {
            if (fn->HasReturnValue())
            {
                std::vector<std::byte> buf(fn->GetTotalSize());
                fn->Invoke(instance, buf.data());
            }
            else
            {
                fn->Invoke(instance, nullptr);
            }
        }
        ImGui::PopID();
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

void EditorWindow_Details::DrawVectorElements(const DVectorPropertyBase* vectorProp, void* vectorAddr, DObject* parentObject, int depth)
{
    if (!vectorProp || !vectorAddr)
        return;

    constexpr int kMaxDepth = 8;
    const DProperty* innerProp = vectorProp->GetInnerProperty();
    if (!innerProp)
        return;

    const size_t count = vectorProp->GetSize(vectorAddr);
    for (size_t index = 0; index < count; ++index)
    {
        void* elementAddr = vectorProp->GetElementAddress(vectorAddr, index);
        if (!elementAddr)
            continue;

        ImGui::PushID(static_cast<int>(index));
        const std::string label = "[" + std::to_string(index) + "]";

        switch (innerProp->GetPropertyType())
        {
        case EPropertyType::Vector:
        {
            const auto* nestedVector = dynamic_cast<const DVectorPropertyBase*>(innerProp);
            if (!nestedVector || depth >= kMaxDepth)
            {
                DrawReadOnlyProperty(label, innerProp->ToString(elementAddr));
                break;
            }

            if (ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "%s", label.c_str()))
            {
                DrawVectorElements(nestedVector, elementAddr, parentObject, depth + 1);
                ImGui::TreePop();
            }
            break;
        }
        case EPropertyType::ObjectPtr:
        {
            DObject* current = innerProp->GetObjectPointer(elementAddr);

            std::string typeStr = innerProp->GetType();
            if (!typeStr.empty() && typeStr.back() == '*')
                typeStr.pop_back();
            const DClass* targetClass = GetReflectionRegistry().FindClassByName(typeStr);

            auto selection = m_objPtrField.Draw(label.c_str(), current, targetClass, "##ObjPick");

            if (selection.has_value())
            {
                DObject* picked = selection.value();
                auto* ptrProp = const_cast<DObjectPtrPropertyBase*>(
                    static_cast<const DObjectPtrPropertyBase*>(innerProp));

                // Record undo only when this is a top-level vector property on parentObject
                const bool isTopLevel = (vectorAddr == static_cast<void*>(parentObject));
                if (isTopLevel && parentObject && g_editorCore)
                {
                    auto [aId, oId] = g_editorCore->GetIdsForObject(parentObject);
                    if (!aId.IsNull())
                    {
                        nlohmann::json before = PropertyToJson(parentObject, vectorProp, *g_editorCore);
                        ptrProp->ResolvePointer(elementAddr, picked);
                        nlohmann::json after = PropertyToJson(parentObject, vectorProp, *g_editorCore);
                        if (before != after)
                        {
                            auto cmd = std::make_unique<EditorCommand_SetProperty>(
                                aId, oId, std::string(vectorProp->GetName()), std::move(before), std::move(after));
                            EditorCommandContext ctx{ *g_editorCore };
                            g_editorCore->GetCommandManager().Execute(std::move(cmd), ctx);
                        }
                        else
                        {
                            parentObject->MarkDirty();
                        }
                        break;
                    }
                }

                ptrProp->ResolvePointer(elementAddr, picked);
                if (parentObject)
                    parentObject->MarkDirty();
            }
            break;
        }
        default:
            DrawReadOnlyProperty(label, innerProp->ToString(elementAddr));
            break;
        }

        ImGui::PopID();
    }
}

WidgetEditEvent EditorWindow_Details::DrawStructPropertyEditor(DObject* instance, DStructProperty* dsp)
{
    WidgetEditEvent merged;
    DStruct* schema = dsp->GetSchema();
    if (!schema)
    {
        DrawReadOnlyProperty(GetPropertyDisplayName(dsp->GetName()),
            dsp->ToString(dsp->GetValue(instance)));
        return merged;
    }

    void* structBase = dsp->GetValue(instance);
    const std::string displayName = GetPropertyDisplayName(dsp->GetName());

    if (ImGui::TreeNodeEx(displayName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        DrawStructSchemaFields(structBase, schema, merged);
        ImGui::TreePop();
    }

    return merged;
}

void EditorWindow_Details::DrawStructSchemaFields(void* structBase, DStruct* ds, WidgetEditEvent& merged)
{
    if (!ds)
        return;

    if (DStruct* sup = ds->GetSuper())
        DrawStructSchemaFields(structBase, sup, merged);

    for (DProperty* p = ds->GetOwnProperties(); p; p = p->GetNext())
    {
        ImGui::PushID(p->GetName().c_str());

        switch (p->GetPropertyType())
        {
        case EPropertyType::Int:
            merged.Merge(DrawIntPropertyAt(structBase, p));
            break;
        case EPropertyType::Float:
            merged.Merge(DrawFloatPropertyAt(structBase, p));
            break;
        case EPropertyType::Double:
            merged.Merge(DrawDoublePropertyAt(structBase, p));
            break;
        case EPropertyType::Bool:
            merged.Merge(DrawBoolPropertyAt(structBase, p));
            break;
        case EPropertyType::String:
            merged.Merge(DrawStringPropertyAt(structBase, p));
            break;
        case EPropertyType::Struct:
        {
            auto* nested = static_cast<DStructProperty*>(p);
            void* innerBase = nested->GetValue(structBase);
            DStruct* nestedSchema = nested->GetSchema();
            const std::string nestedLabel = GetPropertyDisplayName(p->GetName());
            if (ImGui::TreeNodeEx(nestedLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrawStructSchemaFields(innerBase, nestedSchema, merged);
                ImGui::TreePop();
            }
            break;
        }
        default:
            DrawReadOnlyProperty(GetPropertyDisplayName(p->GetName()),
                p->ToString(p->GetValue(structBase)));
            break;
        }

        ImGui::PopID();
    }
}

WidgetEditEvent EditorWindow_Details::DrawIntPropertyAt(void* container, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    int* val = static_cast<int*>(prop->GetValue(container));

    const float availW = BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    ImGui::SetNextItemWidth(availW);
    const bool changed = ImGui::DragInt("##v", val);
    auto evt = WidgetEditFromLastItem(changed);
    EndPropertyRow();

    return evt;
}

WidgetEditEvent EditorWindow_Details::DrawFloatPropertyAt(void* container, DProperty* prop)
{
    float* val = static_cast<float*>(prop->GetValue(container));
    return m_scalarField.Draw(GetPropertyDisplayName(prop->GetName()).c_str(), val, 0.1f);
}

WidgetEditEvent EditorWindow_Details::DrawDoublePropertyAt(void* container, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    double* val = static_cast<double*>(prop->GetValue(container));

    const float availW = BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    ImGui::SetNextItemWidth(availW);
    const bool changed = ImGui::InputDouble("##v", val, 0.1, 1.0, "%.6f");
    auto evt = WidgetEditFromLastItem(changed);
    EndPropertyRow();

    return evt;
}

WidgetEditEvent EditorWindow_Details::DrawBoolPropertyAt(void* container, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    bool* val = static_cast<bool*>(prop->GetValue(container));

    BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    const bool changed = ImGui::Checkbox("##v", val);
    auto evt = WidgetEditFromLastItem(changed);
    EndPropertyRow();

    return evt;
}

WidgetEditEvent EditorWindow_Details::DrawStringPropertyAt(void* container, DProperty* prop)
{
    const std::string& current = *static_cast<const std::string*>(prop->GetValue(container));
    char buf[1024];
    const size_t len = (std::min)(current.size(), sizeof(buf) - 1);
    memcpy(buf, current.c_str(), len);
    buf[len] = '\0';
    buf[sizeof(buf) - 1] = '\0';

    auto evt = m_stringField.Draw(GetPropertyDisplayName(prop->GetName()).c_str(), buf, sizeof(buf));
    if (evt.valueChanged)
    {
        const std::string newVal(buf);
        std::string* addr = static_cast<std::string*>(prop->GetValue(container));
        *addr = newVal;
    }
    return evt;
}

WidgetEditEvent EditorWindow_Details::DrawIntProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    int* val = static_cast<int*>(prop->GetValue(instance));

    const float availW = BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    ImGui::SetNextItemWidth(availW);
    const bool changed = ImGui::DragInt("##v", val);
    auto evt = WidgetEditFromLastItem(changed);
    EndPropertyRow();

    return evt;
}

WidgetEditEvent EditorWindow_Details::DrawFloatProperty(DObject* instance, DProperty* prop)
{
    float* val = static_cast<float*>(prop->GetValue(instance));
    return m_scalarField.Draw(GetPropertyDisplayName(prop->GetName()).c_str(), val, 0.1f);
}

WidgetEditEvent EditorWindow_Details::DrawDoubleProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    double* val = static_cast<double*>(prop->GetValue(instance));

    const float availW = BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    ImGui::SetNextItemWidth(availW);
    const bool changed = ImGui::InputDouble("##v", val, 0.1, 1.0, "%.6f");
    auto evt = WidgetEditFromLastItem(changed);
    EndPropertyRow();

    return evt;
}

WidgetEditEvent EditorWindow_Details::DrawBoolProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    bool* val = static_cast<bool*>(prop->GetValue(instance));

    BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    const bool changed = ImGui::Checkbox("##v", val);
    auto evt = WidgetEditFromLastItem(changed);
    EndPropertyRow();

    return evt;
}

WidgetEditEvent EditorWindow_Details::DrawStringProperty(DObject* instance, DProperty* prop)
{
    const std::string& current = *static_cast<const std::string*>(prop->GetValue(instance));
    char buf[1024];
    const size_t len = (std::min)(current.size(), sizeof(buf) - 1);
    memcpy(buf, current.c_str(), len);
    buf[len] = '\0';
    buf[sizeof(buf) - 1] = '\0';

    auto evt = m_stringField.Draw(GetPropertyDisplayName(prop->GetName()).c_str(), buf, sizeof(buf));
    if (evt.valueChanged)
    {
        const std::string newVal(buf);
        std::string* addr = static_cast<std::string*>(prop->GetValue(instance));
        *addr = newVal;
    }
    return evt;
}

bool EditorWindow_Details::DrawWStringProperty(DObject* instance, DProperty* prop)
{
    const std::wstring& ws = *static_cast<const std::wstring*>(prop->GetValue(instance));
    DrawReadOnlyProperty(GetPropertyDisplayName(prop->GetName()), StringUtils::WStringToUtf8(ws));
    return false;
}

WidgetEditEvent EditorWindow_Details::DrawVector3Property(DObject* instance, DProperty* prop)
{
    Vector3* val = static_cast<Vector3*>(prop->GetValue(instance));
    return m_vec3Field.Draw(GetPropertyDisplayName(prop->GetName()).c_str(), &val->x, 0.1f);
}

WidgetEditEvent EditorWindow_Details::DrawQuaternionProperty(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    Quaternion* val = static_cast<Quaternion*>(prop->GetValue(instance));

    const float availW = BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    ImGui::SetNextItemWidth(availW);
    const bool changed = ImGui::DragFloat4("##v", &val->x, 0.01f);
    auto evt = WidgetEditFromLastItem(changed);
    EndPropertyRow();

    return evt;
}

WidgetEditEvent EditorWindow_Details::DrawFloat4Property(DObject* instance, DProperty* prop)
{
    void* addr = prop->GetValue(instance);

    if (prop->GetMeta("UIType") == "Color")
    {
        float* val = static_cast<float*>(addr);
        return m_colorField.Draw(GetPropertyDisplayName(prop->GetName()).c_str(), val, true);
    }

    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    auto* val = static_cast<DirectX::XMFLOAT4*>(addr);

    const float availW = BeginPropertyRow(GetPropertyDisplayName(prop->GetName()).c_str(), c);
    ImGui::SetNextItemWidth(availW);
    const bool changed = ImGui::DragFloat4("##v", &val->x, 0.01f);
    auto evt = WidgetEditFromLastItem(changed);
    EndPropertyRow();

    return evt;
}

WidgetEditEvent EditorWindow_Details::DrawFloat4x4Property(DObject* instance, DProperty* prop)
{
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    auto* mat = static_cast<DirectX::XMFLOAT4X4*>(prop->GetValue(instance));
    WidgetEditEvent evt;

    const std::string displayName = GetPropertyDisplayName(prop->GetName());
    if (ImGui::TreeNodeEx(displayName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        float availW = BeginPropertyRow("Row 0", c);
        ImGui::SetNextItemWidth(availW);
        bool r0 = ImGui::DragFloat4("##r0", &mat->_11, 0.01f);
        evt.Merge(WidgetEditFromLastItem(r0));
        EndPropertyRow();

        availW = BeginPropertyRow("Row 1", c);
        ImGui::SetNextItemWidth(availW);
        bool r1 = ImGui::DragFloat4("##r1", &mat->_21, 0.01f);
        evt.Merge(WidgetEditFromLastItem(r1));
        EndPropertyRow();

        availW = BeginPropertyRow("Row 2", c);
        ImGui::SetNextItemWidth(availW);
        bool r2 = ImGui::DragFloat4("##r2", &mat->_31, 0.01f);
        evt.Merge(WidgetEditFromLastItem(r2));
        EndPropertyRow();

        availW = BeginPropertyRow("Row 3", c);
        ImGui::SetNextItemWidth(availW);
        bool r3 = ImGui::DragFloat4("##r3", &mat->_41, 0.01f);
        evt.Merge(WidgetEditFromLastItem(r3));
        EndPropertyRow();

        ImGui::TreePop();
    }

    return evt;
}

bool EditorWindow_Details::DrawObjectPtrProperty(DObject* instance, DProperty* prop, int /*depth*/)
{
    DObject* current = prop->GetObjectPointer(instance);

    std::string typeStr = prop->GetType();
    if (!typeStr.empty() && typeStr.back() == '*')
        typeStr.pop_back();
    const DClass* targetClass = GetReflectionRegistry().FindClassByName(typeStr);

    const std::string displayName = GetPropertyDisplayName(prop->GetName());
    const std::string popupId     = std::string("##ObjPick_") + prop->GetName();

    ImGui::PushID(prop->GetName().c_str());

    auto selection = m_objPtrField.Draw(displayName.c_str(), current, targetClass, popupId.c_str());

    if (selection.has_value())
    {
        DObject* picked = selection.value();
        auto [aId, oId] = g_editorCore->GetIdsForObject(instance);
        if (!aId.IsNull())
        {
            nlohmann::json before = PropertyToJson(instance, prop, *g_editorCore);
            nlohmann::json after  = nullptr;
            if (picked && picked->GetOwningAsset())
            {
                after = {
                    {"assetId",  picked->GetOwningAsset()->GetAssetId().ToString()},
                    {"objectId", picked->GetObjectId().ToString()}
                };
            }
            auto cmd = std::make_unique<EditorCommand_SetProperty>(
                aId, oId, prop->GetName(), std::move(before), std::move(after));
            EditorCommandContext ctx{ *g_editorCore };
            g_editorCore->GetCommandManager().Execute(std::move(cmd), ctx);
        }
    }

    ImGui::PopID();
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
        DrawVectorElements(vectorProp, instance, instance, depth + 1);
        ImGui::TreePop();
    }
    return false;
}
