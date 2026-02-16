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

    ImGui::End();
}
