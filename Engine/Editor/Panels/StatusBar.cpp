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

    ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - EditorTheme::kStH));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, EditorTheme::kStH));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, c.DFloor);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##StatusBar", nullptr, flags);
    ImGui::PopStyleColor();

    // 1px top separator
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddLine(ImVec2(0, 0), ImVec2(io.DisplaySize.x, 0),
        ImGui::ColorConvertFloat4ToU32(c.BDeep), 1.f);

    ImGui::SetCursorPosY((EditorTheme::kStH - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::SetCursorPosX(12);

    // Left items
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);

    // Green status dot
    ImVec2 dotPos = ImGui::GetCursorScreenPos();
    drawList->AddCircleFilled(ImVec2(dotPos.x + 2.5f, dotPos.y + ImGui::GetTextLineHeight() * 0.5f),
        2.5f, ImGui::ColorConvertFloat4ToU32(c.Ok));
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
    ImGui::SameLine(0, 4);

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
    ImGui::SameLine(0, 8);
    ImVec2 divPos = ImGui::GetCursorScreenPos();
    float divY = divPos.y + ImGui::GetTextLineHeight() * 0.5f;
    drawList->AddLine(ImVec2(divPos.x, divY - 5.5f), ImVec2(divPos.x, divY + 5.5f),
        ImGui::ColorConvertFloat4ToU32(c.BMid), 1.f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1 + 4);
    ImGui::SameLine(0, 4);

    // Scene
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Scene");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 4);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TLabel);
    ImGui::Text("%s", sceneName.c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 12);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Objects");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 4);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TLabel);
    ImGui::Text("%d", objectCount);
    ImGui::PopStyleColor();

    if (!selectedName.empty())
    {
        ImGui::SameLine(0, 12);
        ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
        ImGui::Text("Selected");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 4);
        ImGui::PushStyleColor(ImGuiCol_Text, c.AccHi);
        ImGui::Text("%s", selectedName.c_str());
        ImGui::PopStyleColor();
    }

    // Right items — right-aligned
    float rightW = 220.f;
    ImGui::SetCursorPosX(io.DisplaySize.x - rightW - 12);
    ImGui::SetCursorPosY((EditorTheme::kStH - ImGui::GetTextLineHeight()) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Renderer");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 4);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TLabel);
    ImGui::Text("%s", rendererName.c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 12);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Build");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 4);
    ImGui::PushStyleColor(ImGuiCol_Text, c.Ok);
    ImGui::Text("%s", buildConfig.c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine(0, 12);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TGhost);
    ImGui::Text("\xce\x94 %s", engineVersion.c_str());
    ImGui::PopStyleColor();

    ImGui::End();
}
