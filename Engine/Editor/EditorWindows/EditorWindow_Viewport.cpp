#include "Editor/EditorWindows/EditorWindow_Viewport.h"

#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/Commands/EditorCommand_SetProperty.h"
#include "Editor/Commands/PropertyValueIO.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorRenderManager.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/EditorWindows/EditorWindowsLog.h"
#include "Editor/Panels/MainToolbar.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"
#include "Runtime/Core/Time.h"
#include "Runtime/Graphics/Structures/Camera.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include <SDL3/SDL.h>
#include <DirectXMath.h>
#include <ImGuizmo.h>
#include <nlohmann/json.hpp>

#include "SimpleMath.h"

#include <type_traits>

#include <algorithm>

using namespace DeltaEngine;
using namespace DirectX;

static int AllocateViewportSlot()
{
    static int s_nextSlot = 0;
    return s_nextSlot++;
}

EditorWindow_Viewport::EditorWindow_Viewport()
{
    DELTA_ASSERT(g_editor != nullptr);
    m_title = "Viewport";
    m_viewportIndex = AllocateViewportSlot();
    m_sceneTextureId          = g_editor->GetSceneTextureId();

    std::vector<EditorViewportCamera> cameras;
    if (LoadViewportCameras(cameras) && m_viewportIndex < static_cast<int>(cameras.size()))
        m_previewCamera = cameras[m_viewportIndex];
}

EditorWindow_Viewport::~EditorWindow_Viewport()
{
    std::vector<EditorViewportCamera> cameras;
    LoadViewportCameras(cameras);
    if (m_viewportIndex >= static_cast<int>(cameras.size()))
        cameras.resize(static_cast<size_t>(m_viewportIndex) + 1);
    cameras[static_cast<size_t>(m_viewportIndex)] = m_previewCamera;
    SaveViewportCameras(cameras);
}

void EditorWindow_Viewport::SetPreviewCamera(const EditorViewportCamera& cam)
{
    m_previewCamera = cam;
    m_settingsDirty = true;
    m_saveTimer     = 0.f;
}

// ---------------------------------------------------------------------------
// Scene render size
// ---------------------------------------------------------------------------

void EditorWindow_Viewport::UpdateSceneRenderSize(int renderW, int renderH)
{
    if (renderW < 1 || renderH < 1)
        return;

    DELTA_ASSERT(g_editor != nullptr);
    UINT currentW, currentH;
    g_editor->GetSceneRenderSize(currentW, currentH);

    if (static_cast<UINT>(renderW) != currentW || static_cast<UINT>(renderH) != currentH)
        g_editor->SetSceneRenderSize(static_cast<UINT>(renderW), static_cast<UINT>(renderH));
}

// ---------------------------------------------------------------------------
// Fly-cam input — writes only to m_previewCamera, never touches scene state
// ---------------------------------------------------------------------------

void EditorWindow_Viewport::UpdateViewportFlyMode(bool viewportImageHovered)
{
    SDL_Window* window = SDL_GetMouseFocus();
    if (!window)
        window = SDL_GetKeyboardFocus();
    if (!window)
        return;

    const bool rightMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);

    if (!m_flyModeActive && viewportImageHovered && rightMouseDown)
    {
        m_flyModeActive = true;
        SDL_SetWindowRelativeMouseMode(window, true);
        float discardX, discardY;
        SDL_GetRelativeMouseState(&discardX, &discardY);
    }

    if (m_flyModeActive && !rightMouseDown)
    {
        m_flyModeActive = false;
        SDL_SetWindowRelativeMouseMode(window, false);
        return;
    }

    if (!m_flyModeActive)
        return;

    // --- Rotation ---
    float relX = 0.0f, relY = 0.0f;
    SDL_GetRelativeMouseState(&relX, &relY);

    if (relX != 0.0f || relY != 0.0f)
    {
        // Decompose current quaternion to yaw/pitch, apply deltas, recompose.
        SimpleMath::Quaternion currentRot;
        currentRot.x = m_previewCamera.rotation.x;
        currentRot.y = m_previewCamera.rotation.y;
        currentRot.z = m_previewCamera.rotation.z;
        currentRot.w = m_previewCamera.rotation.w;

        SimpleMath::Vector3 euler = currentRot.ToEuler();
        float yawDelta   = relX * m_rotationSensitivity * (XM_PI / 180.0f);
        float pitchDelta = relY * m_rotationSensitivity * (XM_PI / 180.0f);

        float newYaw   = euler.y + yawDelta;
        float newPitch = euler.x + pitchDelta;
        const float pitchLimit = 89.0f * (XM_PI / 180.0f);
        newPitch = std::clamp(newPitch, -pitchLimit, pitchLimit);

        SimpleMath::Quaternion newRot = SimpleMath::Quaternion::CreateFromYawPitchRoll(newYaw, newPitch, 0.0f);
        m_previewCamera.rotation = { newRot.x, newRot.y, newRot.z, newRot.w };
    }

    // --- Translation ---
    float dt   = Time::deltaTime;
    float move = m_movementSpeed * dt;

    XMVECTOR rot  = XMLoadFloat4(&m_previewCamera.rotation);
    XMMATRIX rotM = XMMatrixRotationQuaternion(rot);
    XMVECTOR forward  = XMVector3Normalize(rotM.r[2]);
    XMVECTOR right    = XMVector3Normalize(rotM.r[0]);
    XMVECTOR worldUp  = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    XMVECTOR pos      = XMLoadFloat3(&m_previewCamera.position);

    if (ImGui::IsKeyDown(ImGuiKey_W))
        pos = XMVectorAdd(pos,      XMVectorScale(forward, move));
    if (ImGui::IsKeyDown(ImGuiKey_S))
        pos = XMVectorSubtract(pos, XMVectorScale(forward, move));
    if (ImGui::IsKeyDown(ImGuiKey_A))
        pos = XMVectorSubtract(pos, XMVectorScale(right,   move));
    if (ImGui::IsKeyDown(ImGuiKey_D))
        pos = XMVectorAdd(pos,      XMVectorScale(right,   move));
    if (ImGui::IsKeyDown(ImGuiKey_Q))
        pos = XMVectorSubtract(pos, XMVectorScale(worldUp, move));
    if (ImGui::IsKeyDown(ImGuiKey_E))
        pos = XMVectorAdd(pos,      XMVectorScale(worldUp, move));

    XMStoreFloat3(&m_previewCamera.position, pos);
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void EditorWindow_Viewport::Render(bool& open)
{
    DELTA_ASSERT(g_editor != nullptr);
    if (!ImGui::Begin(GetImGuiTitle(), &open))
    {
        g_editor->ClearActiveRenderCamera();
        ImGui::End();
        return;
    }

    // --- Toolbar row ---
    const char* comboPreview = GetResolutionPresetLabel(m_resolution);
    bool resolutionChanged = false;
    ImGui::PushItemWidth(300);
    if (ImGui::BeginCombo("Resolution", comboPreview, 0))
    {
        for (int i = 0; i < static_cast<int>(ViewportResolution::Count); ++i)
        {
            ViewportResolution r = static_cast<ViewportResolution>(i);
            if (ImGui::Selectable(GetResolutionPresetLabel(r), m_resolution == r))
            {
                m_resolution = r;
                resolutionChanged = true;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::PushItemWidth(300);
    ImGui::SliderFloat("Zoom", &m_zoom, 0.01f, 5.0f, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopItemWidth();

    ImGui::SameLine();
    ImGui::PushItemWidth(240);
    if (ImGui::BeginCombo("Camera##vp", "Preview Camera"))
    {
        bool changed = false;
        changed |= ImGui::SliderFloat("FOV",  &m_previewCamera.fov,       10.f, 170.f);
        changed |= ImGui::InputFloat("Near",  &m_previewCamera.nearPlane, 0.f, 0.f, "%.4f");
        changed |= ImGui::InputFloat("Far",   &m_previewCamera.farPlane,  0.f, 0.f, "%.1f");

        m_previewCamera.nearPlane = std::max(0.001f, m_previewCamera.nearPlane);
        m_previewCamera.farPlane  = std::max(m_previewCamera.nearPlane + 0.001f, m_previewCamera.farPlane);

        if (changed)
        {
            m_settingsDirty = true;
            m_saveTimer = 0.f;
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    // Debounced settings save — at most once per second after a change.
    m_saveTimer += Time::deltaTime;
    if (m_settingsDirty && m_saveTimer >= 1.0f)
    {
        std::vector<EditorViewportCamera> cameras;
        LoadViewportCameras(cameras);
        if (m_viewportIndex >= static_cast<int>(cameras.size()))
            cameras.resize(m_viewportIndex + 1);
        cameras[m_viewportIndex] = m_previewCamera;
        SaveViewportCameras(cameras);
        m_settingsDirty = false;
        m_saveTimer = 0.f;
    }

    // --- Render size ---
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    float imageAreaW = availSize.x;
    float imageAreaH = std::max(1.0f, availSize.y);

    int renderW, renderH;
    if (m_resolution == ViewportResolution::FreeAspect)
    {
        renderW = std::max(1, static_cast<int>(imageAreaW));
        renderH = std::max(1, static_cast<int>(imageAreaH));
        UpdateSceneRenderSize(renderW, renderH);
    }
    else
    {
        if (!TryGetResolutionPresetSize(m_resolution, renderW, renderH))
        {
            DLOG(LogEditorWindows, ELogLevel::Warning,
                "Ignored invalid ViewportResolution raw value {} — using fallback 1×1 render size (expected < Count)",
                static_cast<std::underlying_type_t<ViewportResolution>>(m_resolution));
            renderW = 1;
            renderH = 1;
        }
        UpdateSceneRenderSize(renderW, renderH);
    }

    UINT actualRenderW, actualRenderH;
    g_editor->GetSceneRenderSize(actualRenderW, actualRenderH);
    float texW = static_cast<float>(actualRenderW);
    float texH = static_cast<float>(actualRenderH);

    // --- Push preview camera override for the next frame ---
    if (texW > 0 && texH > 0)
    {
        ActiveRenderCamera arc = m_previewCamera.BuildActiveRenderCamera(texW, texH);
        g_editor->SetActiveRenderCamera(arc);
    }

    // --- Zoom ---
    float fitZoom = 1.0f;
    if (imageAreaW > 0 && imageAreaH > 0 && texW > 0 && texH > 0)
    {
        float fitX = imageAreaW / texW;
        float fitY = imageAreaH / texH;
        fitZoom = std::max(0.01f, (fitX < fitY) ? fitX : fitY);
    }
    const float minZoom = fitZoom;
    const float maxZoom = 5.0f;

    if (resolutionChanged || m_lastResolution != m_resolution)
    {
        m_zoom = minZoom;
        m_lastResolution = m_resolution;
    }
    m_zoom = std::clamp(m_zoom, minZoom, maxZoom);

    // --- Scene image ---
    bool viewportImageHovered = false;
    if (m_sceneTextureId && texW > 0 && texH > 0 && availSize.x > 0 && availSize.y > 0)
    {
        ImVec2 displaySize(texW * m_zoom, texH * m_zoom);
        ImGui::Image(m_sceneTextureId, displaySize);
        viewportImageHovered = ImGui::IsItemHovered();

        const ImVec2 imageMin = ImGui::GetItemRectMin();
        DrawGizmo(imageMin, displaySize, texW, texH);
    }

    if (!ImGuizmo::IsUsing())
        UpdateViewportFlyMode(viewportImageHovered);

    ImGui::End();
}

// ---------------------------------------------------------------------------
// Gizmo
// ---------------------------------------------------------------------------

namespace
{
SceneComponent* ResolveGizmoTarget(EditorCore& core)
{
    EditorSelectionState* sel = core.GetSelectionState();
    if (!sel)
        return nullptr;

    DPrimaryAsset* sceneAsset = core.GetActiveSceneAsset();
    if (!sceneAsset)
        return nullptr;
    const AssetId& assetId = sceneAsset->GetAssetId();

    if (sel->HasComponentSelection())
    {
        const auto& ids = sel->GetSelectedComponents();
        if (!ids.empty())
        {
            if (auto* sc = core.ResolveObject<SceneComponent>(assetId, ids.front()))
                return sc;
        }
    }

    if (sel->HasGameObjectSelection())
    {
        const auto& ids = sel->GetSelectedGameObjects();
        if (!ids.empty())
        {
            if (auto* go = core.ResolveObject<GameObject>(assetId, ids.front()))
                return go->GetRootSceneComponent();
        }
    }

    return nullptr;
}

ImGuizmo::OPERATION ToolToOperation(EEditorTransformTool tool)
{
    switch (tool)
    {
    case EEditorTransformTool::Move:   return ImGuizmo::TRANSLATE;
    case EEditorTransformTool::Rotate: return ImGuizmo::ROTATE;
    case EEditorTransformTool::Scale:  return ImGuizmo::SCALE;
    case EEditorTransformTool::Select:
        DELTA_UNREACHABLE();
    default:
        DELTA_CHECK_MSG(false,
            "EEditorTransformTool value {} unexpected (expected Move, Rotate, or Scale)",
            static_cast<int>(tool));
        return ImGuizmo::TRANSLATE;
    }
}
} // namespace

void EditorWindow_Viewport::DrawGizmo(const ImVec2& imageMin, const ImVec2& imageSize, float texW, float texH)
{
    if (!g_editorCore || !g_editor)
        return;

    EditorRenderManager* rm = g_editor->GetRenderManager();
    if (!rm)
        return;

    const EEditorTransformTool tool = rm->GetMainToolbar().GetTransformTool();

    if (tool == EEditorTransformTool::Select)
    {
        // Selection tool — no gizmo. Clear any stale in-flight drag state.
        if (m_gizmoEditing)
        {
            m_gizmoEditing = false;
            m_gizmoEditTarget = nullptr;
            m_gizmoEditBefore = {};
        }
        return;
    }

    SceneComponent* sc = ResolveGizmoTarget(*g_editorCore);
    if (!sc)
    {
        if (m_gizmoEditing)
        {
            m_gizmoEditing = false;
            m_gizmoEditTarget = nullptr;
            m_gizmoEditBefore = {};
        }
        return;
    }

    // Selection churn — drop any in-flight drag for a different target.
    if (m_gizmoEditing && m_gizmoEditTarget != sc)
    {
        m_gizmoEditing = false;
        m_gizmoEditTarget = nullptr;
        m_gizmoEditBefore = {};
    }

    const ImGuizmo::OPERATION op = ToolToOperation(tool);
    ImGuizmo::MODE mode = rm->GetMainToolbar().IsLocalSpace() ? ImGuizmo::LOCAL : ImGuizmo::WORLD;
    if (op == ImGuizmo::SCALE)
        mode = ImGuizmo::LOCAL;

    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetRect(imageMin.x, imageMin.y, imageSize.x, imageSize.y);

    // Build view + projection using the same helper that feeds the runtime camera.
    const CameraCB cb = m_previewCamera.BuildCameraCB(texW, texH);
    // CameraCB stores matrices pre-transposed for HLSL; ImGuizmo expects row-major, so undo the transpose.
    XMFLOAT4X4 view, proj;
    XMStoreFloat4x4(&view, XMMatrixTranspose(cb.viewMatrix));
    XMStoreFloat4x4(&proj, XMMatrixTranspose(cb.projectionMatrix));

    XMFLOAT4X4 worldMat;
    XMStoreFloat4x4(&worldMat, sc->GetWorldTransform());

    const bool wasUsing = m_gizmoEditing;

    ImGuizmo::Manipulate(
        &view.m[0][0],
        &proj.m[0][0],
        op,
        mode,
        &worldMat.m[0][0]);

    const bool isUsing = ImGuizmo::IsUsing();

    // Edit begin — snapshot current local transform JSON for undo.
    if (isUsing && !wasUsing)
    {
        DClass* dc = sc->GetClass();
        DProperty* ltProp = dc ? dc->FindPropertyByName("m_localTransform") : nullptr;
        if (ltProp)
            m_gizmoEditBefore = PropertyToJson(sc, ltProp);
        m_gizmoEditing = true;
        m_gizmoEditTarget = sc;
    }

    // Apply manipulated world matrix back as a local transform while the gizmo is held.
    if (isUsing)
    {
        const XMMATRIX newWorld = XMLoadFloat4x4(&worldMat);

        XMMATRIX parentWorld = XMMatrixIdentity();
        if (SceneComponent* parent = sc->GetParent())
            parentWorld = parent->GetWorldTransform();

        XMVECTOR det;
        const XMMATRIX invParent = XMMatrixInverse(&det, parentWorld);
        const XMMATRIX newLocal = newWorld * invParent;

        XMVECTOR s, r, t;
        if (XMMatrixDecompose(&s, &r, &t, newLocal))
        {
            switch (op)
            {
            case ImGuizmo::TRANSLATE:
                sc->SetLocalPosition(t);
                break;
            case ImGuizmo::ROTATE:
                sc->SetLocalRotation(r);
                break;
            case ImGuizmo::SCALE:
                sc->SetLocalScale(SimpleMath::Vector3(XMVectorGetX(s), XMVectorGetY(s), XMVectorGetZ(s)));
                break;
            default:
                break;
            }
        }
    }

    // Edit end — emit an undoable SetProperty command on m_localTransform.
    if (!isUsing && wasUsing)
    {
        DClass* dc = sc->GetClass();
        DProperty* ltProp = dc ? dc->FindPropertyByName("m_localTransform") : nullptr;
        if (ltProp)
        {
            nlohmann::json valueAfter = PropertyToJson(sc, ltProp);
            if (m_gizmoEditBefore != valueAfter)
            {
                auto [assetId, objectId] = g_editorCore->GetIdsForObject(sc);
                auto cmd = std::make_unique<EditorCommand_SetProperty>(
                    assetId, objectId,
                    std::string("m_localTransform"),
                    std::move(m_gizmoEditBefore),
                    std::move(valueAfter));
                EditorCommandContext ctx{ *g_editorCore };
                g_editorCore->GetCommandManager().Execute(std::move(cmd), ctx);
            }
        }
        m_gizmoEditing = false;
        m_gizmoEditTarget = nullptr;
        m_gizmoEditBefore = {};
    }
}
