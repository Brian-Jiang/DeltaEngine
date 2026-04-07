#include "Editor/EditorWindows/EditorWindow_Viewport.h"

#include <algorithm>
#include <SDL3/SDL.h>
#include <DirectXMath.h>

#include "Runtime/Core/Time.h"
#include "Runtime/Graphics/Structures/Camera.h"
#include "Editor/EditorMain.h"
#include "SimpleMath.h"

using namespace DeltaEngine;
using namespace DirectX;

// ---------------------------------------------------------------------------
// Static viewport index counter
// ---------------------------------------------------------------------------
static int s_nextViewportIndex = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Builds a CameraCB from an EditorViewportCamera for a given render size.
static CameraCB BuildCameraCB(const EditorViewportCamera& cam, float w, float h)
{
    XMVECTOR rot  = XMLoadFloat4(&cam.rotation);
    XMMATRIX rotM = XMMatrixRotationQuaternion(rot);
    XMVECTOR fwd  = XMVector3Normalize(rotM.r[2]);
    XMVECTOR up   = XMVector3Normalize(rotM.r[1]);
    XMVECTOR pos  = XMLoadFloat3(&cam.position);

    CameraCB cb{};
    cb.viewMatrix       = XMMatrixLookToLH(pos, fwd, up);
    cb.projectionMatrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(cam.fov),
        cam.GetAspectRatio(w, h),
        cam.nearPlane,
        cam.farPlane);
    cb.position = pos;
    return cb;
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

EditorWindow_Viewport::EditorWindow_Viewport()
{
    m_title = "Viewport";
    m_sceneTextureId = g_editor->GetSceneTextureId();
    m_viewportIndex = s_nextViewportIndex++;

    std::vector<EditorViewportCamera> cameras;
    if (LoadViewportCameras(cameras) && m_viewportIndex < static_cast<int>(cameras.size()))
        m_previewCamera = cameras[m_viewportIndex];
}

EditorWindow_Viewport::~EditorWindow_Viewport()
{
    // Grow the list to hold at least m_viewportIndex + 1 entries so we don't
    // lose state for other viewports that haven't been destroyed yet.
    std::vector<EditorViewportCamera> cameras;
    LoadViewportCameras(cameras);
    if (m_viewportIndex >= static_cast<int>(cameras.size()))
        cameras.resize(m_viewportIndex + 1);
    cameras[m_viewportIndex] = m_previewCamera;
    SaveViewportCameras(cameras);

    --s_nextViewportIndex;
}

// ---------------------------------------------------------------------------
// Scene render size
// ---------------------------------------------------------------------------

void EditorWindow_Viewport::UpdateSceneRenderSize(int renderW, int renderH)
{
    if (renderW < 1 || renderH < 1)
        return;

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
    if (!ImGui::Begin(GetImGuiTitle(), &open))
    {
        g_editor->ClearPreviewCameraOverride();
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
        GetResolutionPresetSize(m_resolution, renderW, renderH);
        UpdateSceneRenderSize(renderW, renderH);
    }

    UINT actualRenderW, actualRenderH;
    g_editor->GetSceneRenderSize(actualRenderW, actualRenderH);
    float texW = static_cast<float>(actualRenderW);
    float texH = static_cast<float>(actualRenderH);

    // --- Push preview camera override for the next frame ---
    if (texW > 0 && texH > 0)
    {
        CameraCB cb = BuildCameraCB(m_previewCamera, texW, texH);
        g_editor->SetPreviewCameraOverride(cb);
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
    }
    UpdateViewportFlyMode(viewportImageHovered);

    ImGui::End();
}
