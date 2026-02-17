#pragma once

#include "EngineIncludes.h"

#include <memory>

#include "EditorWindows/EditorWindow.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class Device;
class CommandList;
class RenderTarget;

enum class ViewportResolution
{
    FreeAspect,
    Resolution_1280x720,
    Resolution_1920x1080,
    Resolution_3840x2160,
    Count
};

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
    void UpdateSceneRenderSize(int renderW, int renderH);

    ImTextureID m_sceneTextureId = 0;

    ViewportResolution m_resolution = ViewportResolution::FreeAspect;
    float m_zoom = 1.0f;
    ViewportResolution m_lastResolution = ViewportResolution::FreeAspect;
};

DELTA_ENGINE_NS_END
