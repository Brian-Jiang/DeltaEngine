#include "UIComponents/TypeChip.h"

#include "imgui.h"
#include "imgui_internal.h"

using namespace DeltaEngine;

void TypeChip::Draw(const EditorTheme::ThemeColors& c)
{
    constexpr float kSize = 25.f;
    constexpr const char* kIcon = "\xef\x86\xb2";

    // Vertically center the chip against the current text line height
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    float adjustedY = cursorPos.y + (ImGui::GetTextLineHeight() - kSize) * 0.5f;
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, adjustedY));

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##chip", ImVec2(kSize, kSize));
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImVec2 pMin = pos;
    ImVec2 pMax = ImVec2(pos.x + kSize, pos.y + kSize);

    // Background fill — CMesh @ 0.07 alpha
    ImVec4 bgColor = c.CMesh;
    bgColor.w = 0.07f;
    dl->AddRectFilled(pMin, pMax, ImGui::ColorConvertFloat4ToU32(bgColor), 3.f);

    // Border — CMesh @ 0.16 alpha
    ImVec4 borderColor = c.CMesh;
    borderColor.w = 0.16f;
    dl->AddRect(pMin, pMax, ImGui::ColorConvertFloat4ToU32(borderColor), 3.f, 0, 1.f);

    // Icon "◈" — CMesh full color, 18.5px, centered
    ImVec2 textSize = ImGui::GetFont()->CalcTextSizeA(18.5f, FLT_MAX, 0.f, kIcon);
    ImVec2 textPos = ImVec2(
        pos.x + (kSize - textSize.x) * 0.5f,
        pos.y + (kSize - textSize.y) * 0.5f);
    dl->AddText(ImGui::GetFont(), 18.5f, textPos,
        ImGui::ColorConvertFloat4ToU32(c.CMesh), kIcon);
}
