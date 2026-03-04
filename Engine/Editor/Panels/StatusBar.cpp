#include "Panels/StatusBar.h"

#include "EditorMain.h"
#include "Style/EditorTheme.h"

#include "imgui.h"

using namespace DeltaEngine;

void StatusBar::Draw()
{
    ImGuiIO& io = ImGui::GetIO();
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    const float fs  = ImGui::GetFontSize();
    const float pad = ImGui::GetStyle().ItemSpacing.x;

    ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - EditorTheme::StH()));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, EditorTheme::StH()));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, c.DFloor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##StatusBar", nullptr, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    // 1px top separator
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    drawList->AddLine(
        ImVec2(winPos.x,                    winPos.y),
        ImVec2(winPos.x + io.DisplaySize.x, winPos.y),
        ImGui::ColorConvertFloat4ToU32(c.BDeep), 1.f);

    ImGui::SetCursorPosY((EditorTheme::StH() - fs) * 0.5f);
    ImGui::SetCursorPosX(pad);

    // Left items
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);

    // Green status dot
    ImVec2 dotPos = ImGui::GetCursorScreenPos();
    drawList->AddCircleFilled(ImVec2(dotPos.x + fs * 0.2f, dotPos.y + ImGui::GetTextLineHeight() * 0.5f),
        fs * 0.2f, ImGui::ColorConvertFloat4ToU32(c.Ok));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad);
    ImGui::SameLine(0, pad * 0.5f);

    // "Ready" (bold)
    ImGui::PopStyleColor();
    if (theme->GetBoldFont())
        ImGui::PushFont(theme->GetBoldFont());
    ImGui::PushStyleColor(ImGuiCol_Text, c.TLabel);
    ImGui::Text("Ready");
    ImGui::PopStyleColor();
    if (theme->GetBoldFont())
        ImGui::PopFont();

    // Vertical divider
    ImGui::SameLine(0, pad);
    ImVec2 divPos = ImGui::GetCursorScreenPos();
    float divY = divPos.y + ImGui::GetTextLineHeight() * 0.5f;
    drawList->AddLine(ImVec2(divPos.x, divY - fs * 0.4f), ImVec2(divPos.x, divY + fs * 0.4f),
        ImGui::ColorConvertFloat4ToU32(c.BMid), 1.f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1.f + pad * 0.5f);
    ImGui::SameLine(0, pad * 0.5f);

    // Scene
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Scene");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, pad * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TLabel);
    ImGui::Text("%s", sceneName.c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine(0, pad);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Objects");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, pad * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TLabel);
    ImGui::Text("%d", objectCount);
    ImGui::PopStyleColor();

    if (!selectedName.empty())
    {
        ImGui::SameLine(0, pad);
        ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
        ImGui::Text("Selected");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, pad * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, c.AccHi);
        ImGui::Text("%s", selectedName.c_str());
        ImGui::PopStyleColor();
    }

    // Right items — right-aligned
    float rightW = fs * 17.f;
    ImGui::SetCursorPosX(io.DisplaySize.x - rightW - pad);
    ImGui::SetCursorPosY((EditorTheme::StH() - fs) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Renderer");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, pad * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TLabel);
    ImGui::Text("%s", rendererName.c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine(0, pad);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Build");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, pad * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, c.Ok);
    ImGui::Text("%s", buildConfig.c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine(0, pad);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TGhost);
    ImGui::Text("\xce\x94 %s", engineVersion.c_str());
    ImGui::PopStyleColor();

    ImGui::End();
}
