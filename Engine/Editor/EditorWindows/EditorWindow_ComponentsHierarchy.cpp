#include "Editor/EditorWindows/EditorWindow_ComponentsHierarchy.h"

#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Core/DComponent.h"

#include "imgui.h"

using namespace DeltaEngine;

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

    auto selectionState = g_editor->GetSelectionState();
    auto contextGameObject = selectionState->GetContextGameObject();

    if (!contextGameObject)
    {
        ImGui::TextDisabled("Select a GameObject to view its components");
        ImGui::End();
        return;
    }

    // GameObject name as header
    ImGui::TextUnformatted(contextGameObject->GetName().c_str());
    ImGui::Separator();

    auto rootSceneComponent = contextGameObject->GetRootSceneComponent();
    const auto& regularComponents = contextGameObject->GetComponents();

    if (rootSceneComponent)
    {
        RenderSceneComponentTree(rootSceneComponent);
    }

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
    {
        ImGui::TextDisabled("No components");
    }

    ImGui::End();
}

void EditorWindow_ComponentsHierarchy::RenderSceneComponentTree(std::shared_ptr<SceneComponent> sceneComponent)
{
    if (!sceneComponent)
        return;

    const auto& children = sceneComponent->GetChildren();
    bool hasChildren = !children.empty();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if (!hasChildren)
        flags |= ImGuiTreeNodeFlags_Leaf;

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
        flags |= ImGuiTreeNodeFlags_Selected;

    bool open = ImGui::TreeNodeEx(sceneComponent->GetName().c_str(), flags);

    if (ImGui::IsItemClicked())
    {
        selectionState->SelectComponent(sceneComponent);
    }

    if (open)
    {
        for (const auto& child : children)
        {
            RenderSceneComponentTree(child);
        }
        ImGui::TreePop();
    }
}

void EditorWindow_ComponentsHierarchy::RenderRegularComponents(const std::vector<std::shared_ptr<DComponent>>& components)
{
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

        bool open = ImGui::TreeNodeEx(component->GetName().c_str(), flags);

        if (ImGui::IsItemClicked())
        {
            selectionState->SelectComponent(component);
        }

        if (open)
        {
            ImGui::TreePop();
        }
    }
}
