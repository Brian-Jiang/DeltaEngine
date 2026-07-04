#include "UIComponents/PropertyWidgets/ObjectPtrField.h"

#include "UIComponents/PropertyWidgets/PropertyWidgetUtil.h"
#include "UIComponents/UIComponentsEditorTheme.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/Style/EditorTheme.h"

#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include "imgui.h"

#include <cctype>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

using namespace DeltaEngine;

namespace
{
std::string GetObjectDisplayName(DObject* obj)
{
    if (!obj)
        return "(null)";
    DClass* cls = obj->GetClass();
    if (cls)
    {
        DProperty* nameProp = cls->FindPropertyByName("m_name");
        if (nameProp && nameProp->GetPropertyType() == EPropertyType::String)
        {
            const auto& name = *static_cast<const std::string*>(nameProp->GetValue(obj));
            if (!name.empty())
                return name;
        }
    }
    return cls ? cls->GetName() : "Unknown";
}

std::string GetObjectFullDisplayName(DObject* obj)
{
    if (!obj)
        return "(null)";

    const std::string objName = GetObjectDisplayName(obj);

    if (auto* comp = dynamic_cast<DComponent*>(obj))
    {
        if (GameObject* go = comp->GetGameObject())
        {
            const std::string& goName = go->GetName();
            if (!goName.empty())
                return goName + "/" + objName;
        }
    }

    if (DPrimaryAsset* asset = obj->GetOwningAsset())
    {
        if (g_editorCore)
        {
            if (EditorAssetDatabase* db = g_editorCore->GetAssetDatabase())
            {
                std::filesystem::path p = db->GetAssetPath(asset->GetAssetId());
                std::string            stem = p.stem().stem().string();
                if (!stem.empty())
                    return stem + "/" + objName;
            }
        }
    }

    return objName;
}
}

std::optional<DObject*> ObjectPtrField::Draw(
    const char* label, DObject* current, const DClass* targetClass, const char* popupId)
{
    if (!label || !popupId || !targetClass)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "ObjectPtrField::Draw: expected non-null label, popupId, and targetClass (label={}, popupId={}, targetClass={})",
            static_cast<const void*>(label), static_cast<const void*>(popupId), static_cast<const void*>(targetClass));
        return std::nullopt;
    }

    if (!g_editorCore)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "ObjectPtrField::Draw: g_editorCore is null (expected active EditorCore)");
        return std::nullopt;
    }

    EditorTheme* theme = ResolveUIComponentsEditorTheme();
    if (!theme)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "ObjectPtrField::Draw: no EditorTheme (expected g_editor or UIComponents test theme override)");
        return std::nullopt;
    }

    const auto& c    = theme->colors;
    ImDrawList* dl   = ImGui::GetWindowDrawList();
    if (!dl)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "ObjectPtrField::Draw: ImGui::GetWindowDrawList() returned null (expected active window)");
        return std::nullopt;
    }

    std::optional<DObject*> result;

    const float fh = ImGui::GetFrameHeight();
    const float fs = ImGui::GetFontSize();

    const std::string currentName = GetObjectFullDisplayName(current);

    float  availW  = BeginPropertyRow(label, c);
    ImVec2 slotPos = ImGui::GetCursorScreenPos();

    dl->AddRectFilled(slotPos, {slotPos.x + availW, slotPos.y + fh},
        ImGui::ColorConvertFloat4ToU32(c.DInput), 3.f);
    dl->AddRect(slotPos, {slotPos.x + availW, slotPos.y + fh},
        ImGui::ColorConvertFloat4ToU32(c.BLight), 3.f);

    ImGui::SetCursorScreenPos(slotPos);
    const bool slotClicked = ImGui::InvisibleButton("##slot", {availW, fh});

    if (ImGui::IsItemHovered())
        dl->AddRectFilled(slotPos, {slotPos.x + availW, slotPos.y + fh},
            IM_COL32(255, 255, 255, 20), 3.f);

    dl->AddText({slotPos.x + 4.f, slotPos.y + (fh - fs) * 0.5f},
        ImGui::ColorConvertFloat4ToU32(current ? c.TPrimary : c.TDim),
        currentName.c_str());

    if (slotClicked)
    {
        m_pickerFilter[0] = '\0';
        ImGui::OpenPopup(popupId);
    }

    EndPropertyRow();

    availW          = BeginPropertyRow("", c);
    const float btnW = (availW - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

    ImGui::BeginDisabled(current == nullptr);

    if (ImGui::Button("Select", {btnW, 0.f}))
    {
        EditorSelectionState* sel = g_editorCore->GetSelectionState();
        if (!sel)
        {
            DLOG(LogUIComponents, ELogLevel::Warning,
                "ObjectPtrField::Draw: GetSelectionState() returned null (cannot apply Select)");
        }
        else
        {
            if (dynamic_cast<GameObject*>(current))
                sel->SetSelectedGameObject(current->GetObjectId());
            else if (dynamic_cast<DComponent*>(current))
                sel->SetSelectedComponent(current->GetObjectId());
            else if (DPrimaryAsset* asset = current->GetOwningAsset())
                sel->SetSelectedAsset(asset->GetAssetId());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear", {btnW, 0.f}))
        result = static_cast<DObject*>(nullptr);

    ImGui::EndDisabled();
    EndPropertyRow();

    ImGui::SetNextWindowSize({320.f, 420.f}, ImGuiCond_Appearing);
    if (ImGui::BeginPopup(popupId))
    {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 30.f);
        ImGui::InputText("##filter", m_pickerFilter, sizeof(m_pickerFilter));
        ImGui::SameLine();
        if (ImGui::Button("x", {24.f, 0.f}))
            m_pickerFilter[0] = '\0';

        ImGui::Separator();

        struct CandidateGroup
        {
            std::string           groupName;
            DObject*              groupObject = nullptr;
            std::vector<DObject*> items;
        };
        std::vector<CandidateGroup> groups;

        if (DWorld* world = g_editorCore->GetWorld())
        {
            for (GameObject* go : world->GetGameObjects())
            {
                if (!go)
                    continue;
                DClass* goClass = go->GetClass();
                CandidateGroup grp;
                grp.groupName = go->GetName().empty() ? "GameObject" : go->GetName();
                grp.groupObject =
                    (goClass && targetClass && goClass->IsChildOf(targetClass)) ? go : nullptr;

                for (SceneComponent* sc : go->GetSceneComponents())
                {
                    if (!sc)
                        continue;
                    DClass* scClass = sc->GetClass();
                    if (scClass && targetClass && scClass->IsChildOf(targetClass))
                        grp.items.push_back(sc);
                }
                for (DComponent* comp : go->GetComponents())
                {
                    if (!comp)
                        continue;
                    DClass* compClass = comp->GetClass();
                    if (compClass && targetClass && compClass->IsChildOf(targetClass))
                        grp.items.push_back(comp);
                }

                if (grp.groupObject || !grp.items.empty())
                    groups.push_back(std::move(grp));
            }
        }

        if (EditorAssetDatabase* db = g_editorCore->GetAssetDatabase())
        {
            DPrimaryAsset* sceneAsset = g_editorCore->GetActiveSceneAsset();
            for (const auto& [assetId, entry] : db->GetAllAssets())
            {
                DPrimaryAsset* asset = db->LoadAsset(assetId);
                if (!asset || asset == sceneAsset)
                    continue;

                CandidateGroup grp;
                grp.groupName = entry.m_filePath.stem().stem().string();
                if (grp.groupName.empty())
                    grp.groupName = entry.m_header.m_className;

                for (DObject* obj : asset->GetObjects())
                {
                    if (!obj)
                        continue;
                    DClass* objClass = obj->GetClass();
                    if (objClass && targetClass && objClass->IsChildOf(targetClass))
                        grp.items.push_back(obj);
                }

                if (!grp.items.empty())
                    groups.push_back(std::move(grp));
            }
        }

        std::string filterLower = m_pickerFilter;
        for (char& ch : filterLower)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

        auto matches = [&](const std::string& str) -> bool
        {
            if (filterLower.empty())
                return true;
            std::string s;
            s.reserve(str.size());
            for (unsigned char ch : str)
                s += static_cast<char>(std::tolower(ch));
            return s.find(filterLower) != std::string::npos;
        };

        if (filterLower.empty() || matches("none"))
        {
            if (ImGui::Selectable("(None)", current == nullptr))
            {
                result = static_cast<DObject*>(nullptr);
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Separator();

        for (const CandidateGroup& grp : groups)
        {
            const bool grpNameMatches = matches(grp.groupName);

            bool anyItemMatch = grpNameMatches;
            if (!anyItemMatch)
            {
                for (DObject* item : grp.items)
                    if (matches(GetObjectDisplayName(item)))
                    {
                        anyItemMatch = true;
                        break;
                    }
                if (!grp.groupObject && !anyItemMatch)
                    continue;
                if (grp.groupObject && !anyItemMatch && !grpNameMatches)
                    continue;
            }

            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_DefaultOpen;
            if (grp.groupObject && current == grp.groupObject)
                nodeFlags |= ImGuiTreeNodeFlags_Selected;
            if (grp.items.empty())
                nodeFlags |= ImGuiTreeNodeFlags_Leaf;

            const bool nodeOpen = ImGui::TreeNodeEx(grp.groupName.c_str(), nodeFlags);

            if (grp.groupObject && ImGui::IsItemClicked())
            {
                result = grp.groupObject;
                ImGui::CloseCurrentPopup();
            }

            if (nodeOpen)
            {
                for (DObject* item : grp.items)
                {
                    const std::string itemName = GetObjectDisplayName(item);
                    if (!grpNameMatches && !matches(itemName))
                        continue;

                    ImGui::PushID(item);
                    if (ImGui::Selectable(itemName.c_str(), item == current))
                    {
                        result = item;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }
        }

        ImGui::EndPopup();
    }

    return result;
}