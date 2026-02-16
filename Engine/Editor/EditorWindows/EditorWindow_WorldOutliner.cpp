#include "Editor/EditorWindows/EditorWindow_WorldOutliner.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"

#include "imgui.h"

using namespace DeltaEngine;

namespace
{
    enum class WorldOutlinerColumnID
    {
        Index = 0,
        Name = 1
    };

    static int CompareGameObjects(const ImGuiTableSortSpecs* sortSpecs,
        const std::shared_ptr<GameObject>& a, const std::shared_ptr<GameObject>& b, int indexA, int indexB)
    {
        for (int n = 0; n < sortSpecs->SpecsCount; n++)
        {
            const ImGuiTableColumnSortSpecs* spec = &sortSpecs->Specs[n];
            int delta = 0;
            switch (static_cast<WorldOutlinerColumnID>(spec->ColumnUserID))
            {
            case WorldOutlinerColumnID::Index:
                delta = (indexA - indexB);
                break;
            case WorldOutlinerColumnID::Name:
                delta = a->GetName().compare(b->GetName());
                break;
            default:
                break;
            }
            if (delta != 0)
                return (spec->SortDirection == ImGuiSortDirection_Ascending) ? delta : -delta;
        }
        return (indexA - indexB);
    }
}

EditorWindow_WorldOutliner::EditorWindow_WorldOutliner()
{
}

EditorWindow_WorldOutliner::~EditorWindow_WorldOutliner()
{
}

void EditorWindow_WorldOutliner::Render()
{
    if (!ImGui::Begin(m_title, m_open))
    {
        ImGui::End();
        return;
    }

    if (!g_editor || !g_editor->GetEngine())
    {
        ImGui::TextDisabled("No engine");
        ImGui::End();
        return;
    }

    auto world = g_editor->GetEngine()->GetWorld();
    if (!world)
    {
        ImGui::TextDisabled("No world");
        ImGui::End();
        return;
    }

    const auto& gameObjects = world->GetGameObjects();

    ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInner
        | ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable;

    if (ImGui::BeginTable("WorldOutlinerTable", 2, tableFlags))
    {
        ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 50.0f, static_cast<ImGuiID>(WorldOutlinerColumnID::Index));
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 0.0f, static_cast<ImGuiID>(WorldOutlinerColumnID::Name));
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        if (!m_init || world->IsGameObjectsChanged())
        {
            RefreshSortedIndices(gameObjects);
            m_init = true;
        }

        if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs())
        {
            if (sortSpecs->SpecsDirty)
            {
                RefreshSortedIndices(gameObjects);
                sortSpecs->SpecsDirty = false;
            }
        }

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(m_sortedIndices.size()));
        while (clipper.Step())
        {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
            {
                int idx = m_sortedIndices[row];
                auto go = gameObjects[idx];

                ImGui::TableNextRow();

                bool selected = false;
                if (auto selectionState = g_editor->GetSelectionState())
                {
                    for (const auto& selectedGo : selectionState->GetSelectedGameObjects())
                    {
                        if (selectedGo == go)
                        {
                            selected = true;
                            break;
                        }
                    }
                }

                ImGui::PushID(idx);

                // Column 0: Index
                ImGui::TableSetColumnIndex(0);
                if (ImGui::Selectable(std::to_string(idx).c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    if (auto selectionState = g_editor->GetSelectionState())
                    {
                        selectionState->SelectGameObject(go);
                    }
                }

                // Column 1: Name
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(go->GetName().c_str());

                ImGui::PopID();
            }
        }

        ImGui::EndTable();
    }

    ImGui::End();
}

void EditorWindow_WorldOutliner::RefreshSortedIndices(const std::vector<std::shared_ptr<GameObject>>& gameObjects)
{
    m_sortedIndices.clear();
    m_sortedIndices.reserve(gameObjects.size());
    for (size_t i = 0; i < gameObjects.size(); i++)
    {
        m_sortedIndices.push_back(static_cast<int>(i));
    }

    if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs())
    {
        //if (sortSpecs->SpecsDirty)
        {
            std::sort(m_sortedIndices.begin(), m_sortedIndices.end(),
                [&sortSpecs, &gameObjects](int a, int b)
                      {
                          return CompareGameObjects(sortSpecs, gameObjects[a], gameObjects[b], a, b) < 0;
                      });
            sortSpecs->SpecsDirty = false;
        }
    }
}
