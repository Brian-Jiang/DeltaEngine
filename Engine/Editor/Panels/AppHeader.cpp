#include "Panels/AppHeader.h"

#include "EditorMain.h"
#include "Style/EditorTheme.h"

#include "imgui.h"

using namespace DeltaEngine;

void AppHeader::Draw()
{
    ImGuiIO& io = ImGui::GetIO();
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, EditorTheme::kHdrH));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, c.DFloor);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus
        | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
    ImGui::Begin("##AppHeader", nullptr, flags);
    ImGui::PopStyleColor();

    // 1px bottom separator
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    float sepY = EditorTheme::kHdrH - 1.f;
    drawList->AddLine(ImVec2(0, sepY), ImVec2(io.DisplaySize.x, sepY),
        ImGui::ColorConvertFloat4ToU32(c.BDeep), 1.f);

    // Menu bar with logo and menu items
    if (ImGui::BeginMenuBar())
    {
        // Left group: logo + title + divider
        ImGui::SetCursorPos(ImVec2(12, (EditorTheme::kHdrH - 16) * 0.5f));

        // Logo triangle (~9x16)
        ImVec2 triMin = ImGui::GetCursorScreenPos();
        ImVec2 p1(triMin.x, triMin.y + 16);
        ImVec2 p2(triMin.x + 9, triMin.y + 16);
        ImVec2 p3(triMin.x + 4.5f, triMin.y);
        drawList->AddTriangleFilled(p1, p2, p3, ImGui::ColorConvertFloat4ToU32(c.AccHi));

        ImGui::SetCursorPosX(12 + 9 + 8);
        if (theme->GetBoldFont())
            ImGui::PushFont(theme->GetBoldFont());
        ImGui::Text("Delta Engine");
        if (theme->GetBoldFont())
            ImGui::PopFont();

        // Vertical divider
        ImGui::SameLine(0, 8);
        float divX = ImGui::GetCursorScreenPos().x;
        float divY = ImGui::GetCursorScreenPos().y;
        drawList->AddLine(ImVec2(divX, divY), ImVec2(divX, divY + 16),
            ImGui::ColorConvertFloat4ToU32(c.BMid), 1.f);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1 + 8);

        // Menu items
        if (ImGui::BeginMenu("File"))
            ImGui::EndMenu();
        if (ImGui::BeginMenu("Edit"))
            ImGui::EndMenu();
        if (ImGui::BeginMenu("View"))
            ImGui::EndMenu();
        if (ImGui::BeginMenu("Scene"))
            ImGui::EndMenu();
        if (ImGui::BeginMenu("Object"))
            ImGui::EndMenu();
        if (ImGui::BeginMenu("Build"))
            ImGui::EndMenu();
        if (ImGui::BeginMenu("Help"))
            ImGui::EndMenu();

        // Right group — push to right edge
        const float rightGroupWidth = 120.f;
        ImGui::SameLine(io.DisplaySize.x - rightGroupWidth - 8);
        ImGui::SetCursorPosY((EditorTheme::kHdrH - 22) * 0.5f);

        // Build tag
        ImGui::PushStyleColor(ImGuiCol_Text, c.Ok);
        ImGui::PushStyleColor(ImGuiCol_Border, c.BLight);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::Button("Debug x64", ImVec2(70, 22));
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        ImGui::SameLine(0, 8);
        if (ImGui::Button("⚙##Settings", ImVec2(25, 22)))
        {
            // placeholder — no action yet
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Settings");

        ImGui::EndMenuBar();
    }

    ImGui::End();
}
