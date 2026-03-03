#include "Editor/EditorWindows/EditorWindow_Details.h"

#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Core/DComponent.h"

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
