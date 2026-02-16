#include "Editor/EditorWindows/EditorWindow_Viewport.h"

#include <d3dx12.h>

#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Editor/EditorMain.h"

using namespace DeltaEngine;

EditorWindow_Viewport::EditorWindow_Viewport()
{
    m_sceneTextureId = g_editor->GetSceneTextureId();
}

EditorWindow_Viewport::~EditorWindow_Viewport()
{
}

void EditorWindow_Viewport::Render()
{
    if (!ImGui::Begin(m_title, m_open)) {
        ImGui::End();
        return;
    }

    ImVec2 size = ImGui::GetContentRegionAvail();

    if (m_sceneTextureId && size.x > 0 && size.y > 0)
    {
        ImGui::Image(m_sceneTextureId, size);
    }

    ImGui::End();
}