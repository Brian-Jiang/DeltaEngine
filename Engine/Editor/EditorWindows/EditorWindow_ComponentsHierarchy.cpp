#include "Editor/EditorWindows/EditorWindow_ComponentsHierarchy.h"

#include "Editor/Commands/EditorCommand_CreateComponent.h"
#include "Editor/Commands/EditorCommand_DeleteComponent.h"
#include "Editor/Commands/EditorCommand_RenameObject.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/EditorWindows/EditorWindowsLog.h"
#include "Editor/Style/EditorTheme.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Reflection/DClass.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <vector>

using namespace DeltaEngine;

static constexpr const char* kCompIcon   = "\xef\x86\xb2";
static constexpr float       kCompIconSz = 18.f;

EditorWindow_ComponentsHierarchy::EditorWindow_ComponentsHierarchy()
{
    m_title = "Components Hierarchy";
}

EditorWindow_ComponentsHierarchy::~EditorWindow_ComponentsHierarchy()
{
}

void EditorWindow_ComponentsHierarchy::OnRenameCommitted(DObject* obj)
{
    if (!obj || !g_editorCore)
        return;
    auto [assetId, objId] = g_editorCore->GetIdsForObject(obj);
    if (assetId.IsNull() || objId.IsNull())
        return;
    EditorCommandContext ctx{ *g_editorCore };
    g_editorCore->GetCommandManager().Execute(
        std::make_unique<EditorCommand_RenameObject>(assetId, objId, std::string(m_inlineRename.GetBuffer())),
        ctx);
}

void EditorWindow_ComponentsHierarchy::Render(bool& open)
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

    if (g_editor == nullptr)
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "ComponentsHierarchy skipped: global g_editor null (expected EditorMain instance)");
        ImGui::TextDisabled("No editor shell");
        ImGui::End();
        return;
    }

    EditorTheme* theme = g_editor->GetEditorTheme();
    if (theme == nullptr)
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "ComponentsHierarchy: EditorMain returned null EditorTheme (expected themed editor shell)");
        ImGui::TextDisabled("Theme unavailable");
        ImGui::End();
        return;
    }
    const EditorTheme::ThemeColors& c = theme->colors;

    auto* selectionState   = g_editorCore->GetSelectionState();
    auto contextGameObject = selectionState->GetContextGameObject(*g_editorCore);

    if (!contextGameObject)
    {
        ImGui::TextDisabled("Select a GameObject to view its components");

        m_addCompPicker.Draw(c);
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted(contextGameObject->GetName().c_str());
    ImGui::Separator();

    auto rootSceneComponent = contextGameObject->GetRootSceneComponent();
    const auto& regularComponents = contextGameObject->GetComponents();

    if (rootSceneComponent)
        RenderSceneComponentTree(rootSceneComponent);

    if (!regularComponents.empty())
    {
        if (rootSceneComponent)
        {
            ImGui::Separator();
            ImGui::TextDisabled("--- Regular Components ---");
        }
        RenderRegularComponents(regularComponents);
    }

    if (!rootSceneComponent && regularComponents.empty())
        ImGui::TextDisabled("No components");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    {
        const float btnPadX = 10.f;
        const float btnPadY = 4.f;
        const char* addLabel = "+ Add Component";
        ImVec2 labelSz = ImGui::CalcTextSize(addLabel);
        float btnW = labelSz.x + btnPadX * 2.f;
        float btnH = labelSz.y + btnPadY * 2.f;

        float availW = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availW - btnW) * 0.5f);

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(c.Acc.x, c.Acc.y, c.Acc.z, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(c.Acc.x, c.Acc.y, c.Acc.z, 0.22f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(c.Acc.x, c.Acc.y, c.Acc.z, 0.32f));
        ImGui::PushStyleColor(ImGuiCol_Text,          c.Acc);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(btnPadX, btnPadY));

        if (ImGui::Button(addLabel, ImVec2(btnW, btnH)))
        {
            const DClass* compBaseClass = GetReflectionRegistry().FindClassByName("DComponent");
            if (compBaseClass)
            {
                std::vector<const DClass*> compClasses;
                for (const auto& [name, cls] : GetReflectionRegistry().GetAllClasses())
                {
                    if (cls->IsChildOf(compBaseClass) && !cls->IsAbstract())
                        compClasses.push_back(cls);
                }
                m_addCompPicker.Open(std::move(compClasses));
                ImGui::OpenPopup("##ClassPicker");
            }
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
    }

    if (const DClass* picked = m_addCompPicker.Draw(c))
    {
        DPrimaryAsset* sceneAsset = g_editorCore->GetActiveSceneAsset();
        if (sceneAsset)
        {
            EditorCommandContext ctx{ *g_editorCore };
            g_editorCore->GetCommandManager().Execute(
                std::make_unique<EditorCommand_CreateComponent>(
                    sceneAsset->GetAssetId(),
                    contextGameObject->GetObjectId(),
                    std::string(picked->GetName())), ctx);
        }
    }

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput &&
        ImGui::IsKeyPressed(ImGuiKey_F2))
    {
        if (selectionState->GetSelectedComponents().size() == 1)
        {
            const ObjectId cid = selectionState->GetSelectedComponents()[0];
            DObject* obj = nullptr;
            if (DPrimaryAsset* sa = g_editorCore->GetActiveSceneAsset())
                obj = sa->FindObject(cid);
            if (obj)
            {
                m_renameComponentId = cid;
                auto component = dynamic_cast<DComponent*>(obj);
                if (component)
                    m_inlineRename.Begin(component->GetName());
            }
        }
    }

    ImGui::End();
}

void EditorWindow_ComponentsHierarchy::RenderSceneComponentTree(SceneComponent* sceneComponent)
{
    if (!sceneComponent)
        return;

    DELTA_ASSERT(g_editor != nullptr && g_editor->GetEditorTheme() != nullptr);
    const EditorTheme::ThemeColors& c = g_editor->GetEditorTheme()->colors;

    ImGui::PushID(static_cast<void*>(sceneComponent));
    const auto& children = sceneComponent->GetChildren();
    bool hasChildren = !children.empty();
    ImGuiTreeNodeFlags flags = hasChildren ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf;

    auto* selectionState = g_editorCore->GetSelectionState();
    const bool isSelected = selectionState->IsComponentSelected(sceneComponent->GetObjectId());
    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;

    const bool renamingRow = m_inlineRename.IsActive() &&
        sceneComponent->GetObjectId() == m_renameComponentId;
    if (renamingRow)
        flags |= ImGuiTreeNodeFlags_AllowOverlap;

    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float textLineH = ImGui::GetTextLineHeight();
        float iconOffY  = (textLineH - kCompIconSz) * 0.5f;
        dl->AddText(ImGui::GetFont(), kCompIconSz,
            ImVec2(cursorPos.x, cursorPos.y + iconOffY),
            ImGui::ColorConvertFloat4ToU32(c.CMesh), kCompIcon);
        ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + kCompIconSz + 6.f, cursorPos.y));
    }

    char label[256];
    std::snprintf(label, sizeof(label), "%s##%p", sceneComponent->GetName().c_str(), (void *)sceneComponent);
    bool open = ImGui::TreeNodeEx(label, flags);

    if (ImGui::IsItemClicked() && !renamingRow)
    {
        const ObjectId id = sceneComponent->GetObjectId();
        if (ImGui::GetIO().KeyCtrl)
        {
            if (selectionState->IsComponentSelected(id))
                selectionState->RemoveSelectedComponent(id);
            else
                selectionState->AddSelectedComponent(id);
        }
        else
        {
            selectionState->SetSelectedComponent(id);
        }
    }

    if (ImGui::BeginPopupContextItem())
    {
        bool isLastRoot = (sceneComponent == sceneComponent->GetGameObject()->GetRootSceneComponent()
            && sceneComponent->GetGameObject()->GetSceneComponents().size() <= 1);
        std::vector<ContextMenuPopup::Item> items;
        const bool canRename = selectionState->GetSelectedComponents().size() == 1 &&
            selectionState->GetSelectedComponents()[0] == sceneComponent->GetObjectId();
        if (canRename)
        {
            items.push_back({"Rename", [&]() {
                m_renameComponentId = sceneComponent->GetObjectId();
                m_inlineRename.Begin(sceneComponent->GetName());
            }});
        }
        items.push_back({"Destroy", [&, isLastRoot]() {
            if (isLastRoot)
                return;
            GameObject* owner = sceneComponent->GetGameObject();
            if (owner && g_editorCore)
            {
                auto [assetId, compId] = g_editorCore->GetIdsForObject(sceneComponent);
                if (!assetId.IsNull() && !compId.IsNull())
                {
                    EditorCommandContext ctx{ *g_editorCore };
                    g_editorCore->GetCommandManager().Execute(
                        std::make_unique<EditorCommand_DeleteComponent>(
                            assetId, owner->GetObjectId(), compId), ctx);
                }
            }
        }});
        m_destroyCompMenu.Open(std::move(items));
        m_destroyCompMenu.Draw(c);
        ImGui::EndPopup();
    }

    if (renamingRow)
    {
        ImGui::SameLine(ImGui::GetCursorPosX());
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 8.f);
        const auto rr = m_inlineRename.Draw();
        if (rr == EditorInlineRename::Result::Committed)
        {
            OnRenameCommitted(sceneComponent);
            m_renameComponentId = ObjectId::Null();
        }
        else if (rr == EditorInlineRename::Result::Cancelled)
            m_renameComponentId = ObjectId::Null();
    }

    if (open)
    {
        for (const auto& child : children)
            RenderSceneComponentTree(child);
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void EditorWindow_ComponentsHierarchy::RenderRegularComponents(const std::vector<DComponent*>& components)
{
    DELTA_ASSERT(g_editor != nullptr && g_editor->GetEditorTheme() != nullptr);
    const EditorTheme::ThemeColors& c = g_editor->GetEditorTheme()->colors;

    auto* selectionState = g_editorCore->GetSelectionState();

    for (const auto& component : components)
    {
        if (!component)
            continue;

        ImGui::PushID(static_cast<void*>(component));
        const bool isSelected = selectionState->IsComponentSelected(component->GetObjectId());

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
        if (isSelected)
            flags |= ImGuiTreeNodeFlags_Selected;

        const bool renamingRow = m_inlineRename.IsActive() &&
            component->GetObjectId() == m_renameComponentId;
        if (renamingRow)
            flags |= ImGuiTreeNodeFlags_AllowOverlap;

        {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            float textLineH = ImGui::GetTextLineHeight();
            float iconOffY  = (textLineH - kCompIconSz) * 0.5f;
            dl->AddText(ImGui::GetFont(), kCompIconSz,
                ImVec2(cursorPos.x, cursorPos.y + iconOffY),
                ImGui::ColorConvertFloat4ToU32(c.CMesh), kCompIcon);
            ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + kCompIconSz + 6.f, cursorPos.y));
        }

        bool open = ImGui::TreeNodeEx(static_cast<const void*>(component), flags, "##dc%p", (void*)component);
        const bool treeHit = ImGui::IsItemClicked() && !renamingRow;

        if (ImGui::BeginPopupContextItem())
        {
            std::vector<ContextMenuPopup::Item> items;
            const bool canRename = selectionState->GetSelectedComponents().size() == 1 &&
                selectionState->GetSelectedComponents()[0] == component->GetObjectId();
            if (canRename)
            {
                items.push_back({"Rename", [&]() {
                    m_renameComponentId = component->GetObjectId();
                    m_inlineRename.Begin(component->GetName());
                }});
            }
            items.push_back({"Destroy", [&]() {
                GameObject* owner = component->GetGameObject();
                if (owner && g_editorCore)
                {
                    auto [assetId, compId] = g_editorCore->GetIdsForObject(component);
                    if (!assetId.IsNull() && !compId.IsNull())
                    {
                        EditorCommandContext ctx{ *g_editorCore };
                        g_editorCore->GetCommandManager().Execute(
                            std::make_unique<EditorCommand_DeleteComponent>(
                                assetId, owner->GetObjectId(), compId), ctx);
                    }
                }
            }});
            m_destroyCompMenu.Open(std::move(items));
            m_destroyCompMenu.Draw(c);
            ImGui::EndPopup();
        }

        ImGui::SameLine(0.f, 6.f);

        if (renamingRow)
        {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 8.f);
            const auto rr = m_inlineRename.Draw();
            if (rr == EditorInlineRename::Result::Committed)
            {
                OnRenameCommitted(component);
                m_renameComponentId = ObjectId::Null();
            }
            else if (rr == EditorInlineRename::Result::Cancelled)
                m_renameComponentId = ObjectId::Null();
        }
        else
            ImGui::TextUnformatted(component->GetName().c_str());

        bool labelHit = false;
        if (!renamingRow)
            labelHit = ImGui::IsItemClicked();
        if (treeHit || labelHit)
        {
            const ObjectId id = component->GetObjectId();
            if (ImGui::GetIO().KeyCtrl)
            {
                if (selectionState->IsComponentSelected(id))
                    selectionState->RemoveSelectedComponent(id);
                else
                    selectionState->AddSelectedComponent(id);
            }
            else
            {
                selectionState->SetSelectedComponent(id);
            }
        }

        if (open)
            ImGui::TreePop();
        ImGui::PopID();
    }
}
