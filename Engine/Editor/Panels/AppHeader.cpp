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

    // ── Scale units — never use raw pixel literals ──────────────────────────
    //   fh  = one standard widget row height (font + frame padding × 2)
    //   fs  = raw font size
    //   pad = standard item spacing x
    const float fh = ImGui::GetFrameHeight();
    const float fs = ImGui::GetFontSize();
    const float pad = ImGui::GetStyle().ItemSpacing.x;

    // Window height = exactly the menu bar row.
    // NoDecoration removes the title bar (0 height), so the whole window IS
    // the menu bar — no leftover content area that would get clipped.
    const float hdrH = fh;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, hdrH));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, c.DFloor);
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, c.DFloor); // unify colors
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;

    ImGui::Begin("##AppHeader", nullptr, flags);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();

    // ── 1 px bottom separator ────────────────────────────────────────────────
    float sepY = winPos.y + hdrH - 1.f;
    dl->AddLine(
        ImVec2(winPos.x, sepY),
        ImVec2(winPos.x + io.DisplaySize.x, sepY),
        ImGui::ColorConvertFloat4ToU32(c.BDeep), 1.f);

    if (ImGui::BeginMenuBar()) {
        // ── Logo: equilateral triangle ───────────────────────────────────────
        // Side length = font size, height = side * (√3/2)
        // GetCursorScreenPos() inside BeginMenuBar already sits at the correct
        // Y for inline content — do NOT fight it with SetCursorPos Y.
        {
            const float side = fs;
            const float triH = side * 0.866f; // √3/2
            ImVec2 sp = ImGui::GetCursorScreenPos();
            float offY = (fh - triH) * 0.5f - ImGui::GetStyle().FramePadding.y;
            ImVec2 p1(sp.x, sp.y + offY + triH); // bottom-left
            ImVec2 p2(sp.x + side, sp.y + offY + triH); // bottom-right
            ImVec2 p3(sp.x + side * 0.5f, sp.y + offY); // apex
            dl->AddTriangleFilled(p1, p2, p3, ImGui::ColorConvertFloat4ToU32(c.AccHi));
            // Advance cursor past the triangle without ImGui knowing about it
            ImGui::Dummy(ImVec2(side, 0.f));
        }

        // ── Title ────────────────────────────────────────────────────────────
        ImGui::SameLine(0.f, pad * 0.5f);
        if (theme->GetBoldFont())
            ImGui::PushFont(theme->GetBoldFont());
        ImGui::Text("Delta Engine");
        if (theme->GetBoldFont())
            ImGui::PopFont();

        // ── Vertical divider ─────────────────────────────────────────────────
        ImGui::SameLine(0.f, pad);
        {
            ImVec2 sp = ImGui::GetCursorScreenPos();
            dl->AddLine(
                ImVec2(sp.x, sp.y),
                ImVec2(sp.x, sp.y + fs),
                ImGui::ColorConvertFloat4ToU32(c.BMid), 1.f);
            ImGui::Dummy(ImVec2(1.f, 0.f));
        }
        ImGui::SameLine(0.f, pad * 0.5f);

        // ── Menu items ───────────────────────────────────────────────────────
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

        // ── Right group: push to right edge ──────────────────────────────────
        // Measure widths in font-relative units so they resize with the font
        const float btnW = fs * 4.5f; // "Debug x64"
        const float iconW = fh; // square gear button
        const float groupW = btnW + pad * 0.5f + iconW + pad;

        ImGui::SameLine(io.DisplaySize.x - groupW);

        ImGui::PushStyleColor(ImGuiCol_Text, c.Ok);
        ImGui::PushStyleColor(ImGuiCol_Border, c.BLight);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::Button("Debug x64", ImVec2(btnW, 0.f)); // 0 height = auto (fh)
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        ImGui::SameLine(0.f, pad * 0.5f);
        if (ImGui::Button("⚙##Settings", ImVec2(iconW, 0.f))) { /* placeholder */
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Settings");

        ImGui::EndMenuBar();
    }

    ImGui::End();
}
