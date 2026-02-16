#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "EditorWindows/EditorWindow.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;
class RenderTarget;

class EditorWindow_Viewport : public EditorWindow
{
public:
    EditorWindow_Viewport();
    ~EditorWindow_Viewport();

    /// Renders the viewport window. If sceneTextureId is valid, displays the game rendering via ImGui::Image.
    /// When using blit path, commandList is used for resolve/copy; transition and descriptor copy happen in EditorRenderManager.
    void Render() override;

    void SetSceneTexture(ImTextureID textureId) { m_sceneTextureId = textureId; }

    const char* m_title = "Viewport";
    bool* m_open = nullptr;

private:
    ImTextureID m_sceneTextureId = 0;
};

DELTA_ENGINE_NS_END
