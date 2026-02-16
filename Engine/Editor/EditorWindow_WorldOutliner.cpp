#include "Editor/EditorWindow_WorldOutliner.h"

#include "imgui.h"

using namespace DeltaEngine;

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

    if (ImGui::BeginTable("WorldOutlinerTable", 1, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInner))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_None, 0.0f, 0);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        ImGui::EndTable();
    }

    ImGui::End();
}
