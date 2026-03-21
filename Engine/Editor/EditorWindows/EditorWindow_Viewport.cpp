#include "Editor/EditorWindows/EditorWindow_Viewport.h"

#include <algorithm>
#include <SDL3/SDL.h>
#include <DirectXMath.h>

#include "Runtime/EngineMain.h"
#include "Runtime/Core/Camera.h"
#include "Runtime/Core/Time.h"
#include "Editor/EditorMain.h"
#include "SimpleMath.h"

using namespace DeltaEngine;
using namespace DirectX;

EditorWindow_Viewport::EditorWindow_Viewport()
{
    m_sceneTextureId = g_editor->GetSceneTextureId();
}

EditorWindow_Viewport::~EditorWindow_Viewport()
{
}

void EditorWindow_Viewport::UpdateSceneRenderSize(int renderW, int renderH)
{
    if (renderW < 1 || renderH < 1)
        return;

    UINT currentW, currentH;
    g_editor->GetSceneRenderSize(currentW, currentH);

    if (static_cast<UINT>(renderW) != currentW || static_cast<UINT>(renderH) != currentH)
    {
        g_editor->SetSceneRenderSize(static_cast<UINT>(renderW), static_cast<UINT>(renderH));
    }
}

void EditorWindow_Viewport::UpdateViewportFlyMode(bool viewportImageHovered)
{
    EngineMain* engine = g_editor->GetEngine();
    if (!engine)
        return;

    Camera* camera = engine->GetCamera();
    if (!camera)
        return;

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

    float relX = 0.0f, relY = 0.0f;
    SDL_GetRelativeMouseState(&relX, &relY);

    if (relX != 0.0f || relY != 0.0f)
    {
        SimpleMath::Vector3 euler = camera->GetWorldRotation().ToEuler();
        float yawDelta = relX * m_rotationSensitivity * (XM_PI / 180.0f);
        float pitchDelta = relY * m_rotationSensitivity * (XM_PI / 180.0f);

        float newYaw = euler.y + yawDelta;
        float newPitch = euler.x + pitchDelta;
        const float pitchLimit = 89.0f * (XM_PI / 180.0f);
        newPitch = std::clamp(newPitch, -pitchLimit, pitchLimit);

        SimpleMath::Quaternion newRot = SimpleMath::Quaternion::CreateFromYawPitchRoll(newYaw, newPitch, 0.0f);
        camera->SetWorldRotation(newRot);
    }

    float dt = Time::deltaTime;
    float move = m_movementSpeed * dt;
    XMFLOAT3 positionVector = camera->GetWorldPosition();
    XMFLOAT3 forwardVector = camera->GetForward();
    XMFLOAT3 rightVector = camera->GetRight();
    XMVECTOR pos = XMLoadFloat3(&positionVector);
    XMVECTOR forward = XMVector3Normalize(XMLoadFloat3(&forwardVector));
    XMVECTOR right = XMVector3Normalize(XMLoadFloat3(&rightVector));
    XMVECTOR worldUp = XMVectorSet(0, 1, 0, 0);

    if (ImGui::IsKeyDown(ImGuiKey_W))
        pos = XMVectorAdd(pos, XMVectorScale(forward, move));
    if (ImGui::IsKeyDown(ImGuiKey_S))
        pos = XMVectorSubtract(pos, XMVectorScale(forward, move));
    if (ImGui::IsKeyDown(ImGuiKey_A))
        pos = XMVectorSubtract(pos, XMVectorScale(right, move));
    if (ImGui::IsKeyDown(ImGuiKey_D))
        pos = XMVectorAdd(pos, XMVectorScale(right, move));
    if (ImGui::IsKeyDown(ImGuiKey_Q))
        pos = XMVectorSubtract(pos, XMVectorScale(worldUp, move));
    if (ImGui::IsKeyDown(ImGuiKey_E))
        pos = XMVectorAdd(pos, XMVectorScale(worldUp, move));

    XMFLOAT3 newPos;
    XMStoreFloat3(&newPos, pos);
    camera->SetWorldPosition(SimpleMath::Vector3(newPos.x, newPos.y, newPos.z));
}

void EditorWindow_Viewport::Render()
{
    if (!ImGui::Begin(m_title, m_open)) {
        ImGui::End();
        return;
    }

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

    ImVec2 availSize = ImGui::GetContentRegionAvail();
    float imageAreaW = availSize.x;
    float imageAreaH = std::max(1.0f, availSize.y);

    int renderW, renderH;
    if (m_resolution == ViewportResolution::FreeAspect)
    {
        renderW = static_cast<int>(imageAreaW);
        renderH = static_cast<int>(imageAreaH);
        renderW = std::max(1, renderW);
        renderH = std::max(1, renderH);
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

    float fitZoom = 1.0f;
    if (imageAreaW > 0 && imageAreaH > 0 && texW > 0 && texH > 0)
    {
        float fitX = imageAreaW / texW;
        float fitY = imageAreaH / texH;
        fitZoom = (fitX < fitY) ? fitX : fitY;
        fitZoom = std::max(0.01f, fitZoom);
    }
    const float minZoom = fitZoom;
    const float maxZoom = 5.0f;

    if (resolutionChanged || m_lastResolution != m_resolution)
    {
        m_zoom = minZoom;
        m_lastResolution = m_resolution;
    }

    if (m_zoom < minZoom)
        m_zoom = minZoom;
    if (m_zoom > maxZoom)
        m_zoom = maxZoom;

    ImGui::SameLine();
    ImGui::PushItemWidth(500);
    ImGui::SliderFloat("Zoom", &m_zoom, minZoom, maxZoom, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopItemWidth();

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
