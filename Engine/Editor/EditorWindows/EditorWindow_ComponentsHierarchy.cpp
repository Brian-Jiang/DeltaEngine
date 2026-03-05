#include "Editor/EditorWindows/EditorWindow_ComponentsHierarchy.h"

#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/Style/EditorTheme.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Core/DComponent.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Reflection/DClass.h"

#include "imgui.h"
#include "imgui_internal.h"

using namespace DeltaEngine;

// UTF-8 for FA cube glyph U+F1B2 — same as TypeChip
static constexpr const char* kCompIcon   = "\xef\x86\xb2";
static constexpr float       kCompIconSz = 18.f;

EditorWindow_ComponentsHierarchy::EditorWindow_ComponentsHierarchy()
{
}

EditorWindow_ComponentsHierarchy::~EditorWindow_ComponentsHierarchy()
{
}

void EditorWindow_ComponentsHierarchy::Render()
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

    EditorTheme* theme = g_editor->GetEditorTheme();
    const EditorTheme::ThemeColors& c = theme ? theme->colors : EditorTheme::ThemeColors{};

    auto selectionState    = g_editor->GetSelectionState();
    auto contextGameObject = selectionState->GetContextGameObject();

    if (!contextGameObject)
    {
        ImGui::TextDisabled("Select a GameObject to view its components");

        // Still need to draw the popup (it may be open)
        m_addCompPicker.Draw(c);
        ImGui::End();
        return;
    }

    // ── GameObject name header ───────────────────────────────────────────────
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

    // ── "+ Add Component" button ─────────────────────────────────────────────
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

    // Draw the component picker popup
    if (const DClass* picked = m_addCompPicker.Draw(c))
    {
        contextGameObject->AddComponentByClass(picked);
    }

    ImGui::End();
}

void EditorWindow_ComponentsHierarchy::RenderSceneComponentTree(SceneComponent* sceneComponent)
{
    if (!sceneComponent)
        return;

    EditorTheme* theme = g_editor ? g_editor->GetEditorTheme() : nullptr;
    const EditorTheme::ThemeColors& c = theme ? theme->colors : EditorTheme::ThemeColors{};

    const auto& children = sceneComponent->GetChildren();
    bool hasChildren = !children.empty();
    ImGuiTreeNodeFlags flags = hasChildren ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf;

    auto selectionState = g_editor->GetSelectionState();
    bool isSelected = false;
    for (const auto& comp : selectionState->GetSelectedComponents())
    {
        if (comp == sceneComponent)
        {
            isSelected = true;
            break;
        }
    }
    if (isSelected)
        flags |= ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow;

    // Icon drawn before the tree node label via DrawList
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float textLineH = ImGui::GetTextLineHeight();
        float iconOffY  = (textLineH - kCompIconSz) * 0.5f;
        dl->AddText(ImGui::GetFont(), kCompIconSz,
            ImVec2(cursorPos.x, cursorPos.y + iconOffY),
            ImGui::ColorConvertFloat4ToU32(c.CMesh), kCompIcon);
        // Indent cursor past the icon
        ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + kCompIconSz + 6.f, cursorPos.y));
    }

    char label[256];
    std::snprintf(label, sizeof(label), "%s##%p", sceneComponent->GetName().c_str(), (void*)sceneComponent);
    bool open = ImGui::TreeNodeEx(label, flags);

    if (ImGui::IsItemClicked())
        selectionState->SelectComponent(sceneComponent);

    if (open)
    {
        for (const auto& child : children)
            RenderSceneComponentTree(child);
        ImGui::TreePop();
    }
}

void EditorWindow_ComponentsHierarchy::RenderRegularComponents(const std::vector<DComponent*>& components)
{
    EditorTheme* theme = g_editor ? g_editor->GetEditorTheme() : nullptr;
    const EditorTheme::ThemeColors& c = theme ? theme->colors : EditorTheme::ThemeColors{};

    auto selectionState = g_editor->GetSelectionState();

    for (const auto& component : components)
    {
        if (!component)
            continue;

        bool isSelected = false;
        for (const auto& comp : selectionState->GetSelectedComponents())
        {
            if (comp == component)
            {
                isSelected = true;
                break;
            }
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf;
        if (isSelected)
            flags |= ImGuiTreeNodeFlags_Selected;

        // Icon drawn before the tree node label via DrawList
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

        bool open = ImGui::TreeNodeEx(component->GetName().c_str(), flags);

        if (ImGui::IsItemClicked())
            selectionState->SelectComponent(component);

        if (open)
            ImGui::TreePop();
    }
}
