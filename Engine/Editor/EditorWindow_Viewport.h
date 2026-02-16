#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;
class RenderTarget;

class EditorWindow_Viewport
{
public:
    EditorWindow_Viewport();
    ~EditorWindow_Viewport();

    /// Renders the viewport window. If sceneTextureId is valid, displays the game rendering via ImGui::Image.
    /// When using blit path, commandList is used for resolve/copy; transition and descriptor copy happen in EditorRenderManager.
    void Render(std::shared_ptr<CommandList> commandList, std::shared_ptr<RenderTarget> offscreenRenderTarget,
        ImTextureID sceneTextureId);

    const char* m_title = "Viewport";
    bool* m_open = nullptr;
};

DELTA_ENGINE_NS_END
