#include "Editor/EditorWindow_Viewport.h"

#include <d3dx12.h>

#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/DirectX/CommandList.h"

using namespace DeltaEngine;

EditorWindow_Viewport::EditorWindow_Viewport()
{
}

EditorWindow_Viewport::~EditorWindow_Viewport()
{
}

void EditorWindow_Viewport::Render(std::shared_ptr<CommandList> commandList, std::shared_ptr<RenderTarget> offscreenRenderTarget,
    ImTextureID sceneTextureId)
{
    if (!ImGui::Begin(m_title, m_open)) {
        ImGui::End();
        return;
    }

    ImVec2 size = ImGui::GetContentRegionAvail();

    if (sceneTextureId && size.x > 0 && size.y > 0)
    {
        ImGui::Image(sceneTextureId, size);
    }

    ImGui::End();
}