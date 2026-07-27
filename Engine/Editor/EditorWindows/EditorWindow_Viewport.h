#pragma once

#include "EngineIncludes.h"

#include "EditorWindows/EditorWindow.h"
#include "EditorWindows/EditorWindow_ViewportPresets.h"
#include "Editor/EditorViewportCamera.h"
#include "Panels/MainToolbar.h"
#include "Runtime/Core/Delegates/DelegateHandle.h"
#include "imgui.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class EditorCore;
class SceneComponent;

/** Decomposed SceneComponent property a gizmo drag with this tool commits; nullptr for Select. */
DELTAEDITOR_API const char* GizmoTransformPropertyName(EEditorTransformTool tool);

/** Current JSON value of the property `tool` commits; null json when unavailable. */
DELTAEDITOR_API nlohmann::json CaptureGizmoTransformValue(SceneComponent* component, EEditorTransformTool tool);

/**
 * Commits a finished gizmo drag as one undoable SetProperty on the tool's decomposed property.
 * Returns false when the value is unchanged or the edit cannot be resolved.
 */
DELTAEDITOR_API bool CommitGizmoTransformEdit(EditorCore& core, SceneComponent* component,
                                              EEditorTransformTool tool, nlohmann::json valueBefore);

class EditorWindow_Viewport : public EditorWindow
{
public:
    /// Captures the current scene ImGui texture id from the editor singleton.
    /// Loads persisted camera state by viewport index.
    EditorWindow_Viewport();
    /// Saves camera state on destruction.
    ~EditorWindow_Viewport();

    /// Draws the viewport toolbar, scene image, and fly-camera input when applicable.
    void Render(bool& open) override;

    /// Viewport instances are removed from the window list when the user closes them.
    bool ShouldDestroyOnClose() const override { return true; }

    /// Viewport windows are not singletons — multiple instances can be open at once.
    bool IsSingleton() const override { return false; }

    /// ImGui texture id for the scene color target shown in the viewport.
    void SetSceneTexture(ImTextureID textureId) { m_sceneTextureId = textureId; }

    const EditorViewportCamera& GetPreviewCamera() const { return m_previewCamera; }
    void SetPreviewCamera(const EditorViewportCamera& cam);

private:
    void HandleSelectionChanged();
    void UpdateSceneRenderSize(int renderW, int renderH);
    void UpdateViewportFlyMode(bool viewportImageHovered);
    void DrawGizmo(const ImVec2& imageMin, const ImVec2& imageSize, float texW, float texH);
    void ClearGizmoEditState();

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

    bool                 m_gizmoEditing    = false;
    SceneComponent*      m_gizmoEditTarget = nullptr;
    EEditorTransformTool m_gizmoEditTool   = EEditorTransformTool::Select;
    nlohmann::json       m_gizmoEditBefore;

    FDelegateHandle m_onSelectionChangedHandle;
};

DELTA_ENGINE_NS_END
