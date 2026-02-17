#include "Editor/EditorWindows/EditorWindow_Viewport.h"

#include <d3dx12.h>

#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Editor/EditorMain.h"

using namespace DeltaEngine;

namespace
{
    void GetResolutionPresetSize(ViewportResolution preset, int& outW, int& outH)
    {
        switch (preset)
        {
        case ViewportResolution::Resolution_1280x720:  outW = 1280;  outH = 720;  break;
        case ViewportResolution::Resolution_1920x1080: outW = 1920;  outH = 1080; break;
        case ViewportResolution::Resolution_3840x2160: outW = 3840;  outH = 2160; break;
        default: outW = 0; outH = 0; break;
        }
    }

    const char* GetResolutionPresetLabel(ViewportResolution preset)
    {
        switch (preset)
        {
        case ViewportResolution::FreeAspect:           return "Free Aspect";
        case ViewportResolution::Resolution_1280x720:   return "1280 x 720";
        case ViewportResolution::Resolution_1920x1080: return "1920 x 1080";
        case ViewportResolution::Resolution_3840x2160: return "3840 x 2160 (4K)";
        default: return "Unknown";
        }
    }
}

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

void EditorWindow_Viewport::Render()
{
    if (!ImGui::Begin(m_title, m_open)) {
        ImGui::End();
        return;
    }

    // --- Resolution dropdown and Zoom slider ---
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

    // --- Get available size for the scene image (below the toolbar) ---
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    float imageAreaW = availSize.x;
    float imageAreaH = std::max(1.0f, availSize.y);

    // Determine render size
    int renderW, renderH;
    if (m_resolution == ViewportResolution::FreeAspect)
    {
        renderW = static_cast<int>(imageAreaW);
        renderH = static_cast<int>(imageAreaH);
        renderW = std::max(1, renderW);
        renderH = std::max(1, renderH);
        // Free aspect: viewport resize causes render size change
        UpdateSceneRenderSize(renderW, renderH);
    }
    else
    {
        GetResolutionPresetSize(m_resolution, renderW, renderH);
        UpdateSceneRenderSize(renderW, renderH);
    }

    // Refresh render size from editor (in case UpdateSceneRenderSize changed it)
    UINT actualRenderW, actualRenderH;
    g_editor->GetSceneRenderSize(actualRenderW, actualRenderH);
    float texW = static_cast<float>(actualRenderW);
    float texH = static_cast<float>(actualRenderH);

    // Zoom slider: min = fit to viewport, max = 5
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

    // After resolution preset change, set zoom to fit
    if (resolutionChanged || m_lastResolution != m_resolution)
    {
        m_zoom = minZoom;
        m_lastResolution = m_resolution;
    }

    // Clamp zoom to valid range
    if (m_zoom < minZoom)
        m_zoom = minZoom;
    if (m_zoom > maxZoom)
        m_zoom = maxZoom;

    ImGui::SameLine();
    ImGui::PushItemWidth(500);
    ImGui::SliderFloat("Zoom", &m_zoom, minZoom, maxZoom, "%.2fx", ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopItemWidth();

    // Display the scene texture
    if (m_sceneTextureId && texW > 0 && texH > 0 && availSize.x > 0 && availSize.y > 0)
    {
        ImVec2 displaySize(texW * m_zoom, texH * m_zoom);
        ImGui::Image(m_sceneTextureId, displaySize);
    }

    ImGui::End();
}