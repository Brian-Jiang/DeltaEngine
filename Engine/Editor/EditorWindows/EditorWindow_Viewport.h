#pragma once

#include "EngineIncludes.h"

#include "EditorWindows/EditorWindow.h"
#include "EditorWindows/EditorWindow_ViewportPresets.h"
#include "Editor/EditorViewportCamera.h"
#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class EditorWindow_Viewport : public EditorWindow
{
public:
    /// Captures the current scene ImGui texture id from the editor singleton.
    /// Loads persisted camera state by viewport index.
    EditorWindow_Viewport();
    /// Saves camera state on destruction.
    ~EditorWindow_Viewport();

    /// Draws the viewport toolbar, scene image, and fly-camera input when applicable.
    void Render() override;

    /// ImGui texture id for the scene color target shown in the viewport.
    void SetSceneTexture(ImTextureID textureId) { m_sceneTextureId = textureId; }

    /// ImGui window title.
    const char* m_title = "Viewport";
    /// If non-null, ImGui shows a collapse/close widget and writes open state here.
    bool* m_open = nullptr;

private:
    void UpdateSceneRenderSize(int renderW, int renderH);
    void UpdateViewportFlyMode(bool viewportImageHovered);

    ImTextureID m_sceneTextureId = 0;

    ViewportResolution m_resolution = ViewportResolution::FreeAspect;
    float m_zoom = 1.0f;
    ViewportResolution m_lastResolution = ViewportResolution::FreeAspect;

    bool m_flyModeActive = false;
    float m_rotationSensitivity = 0.15f;
    float m_movementSpeed = 150.0f;

    EditorViewportCamera m_previewCamera;
    int m_viewportIndex = 0;
    bool m_settingsDirty = false;
    float m_saveTimer = 0.f;
};

DELTA_ENGINE_NS_END
