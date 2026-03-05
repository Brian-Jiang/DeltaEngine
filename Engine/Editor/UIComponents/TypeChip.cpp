#include "UIComponents/TypeChip.h"

#include "imgui.h"
#include "imgui_internal.h"

using namespace DeltaEngine;

void TypeChip::Draw(const EditorTheme::ThemeColors& c)
{
    constexpr float kSize = 15.f;
    // \xe2\x97\x88 = U+25C8 "◈"
    constexpr const char* kIcon = "\xe2\x97\x88";

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

    // Icon "◈" — CMesh full color, 8.5px, centered
    ImVec2 textSize = ImGui::CalcTextSize(kIcon);
    ImVec2 textPos = ImVec2(
        pos.x + (kSize - textSize.x) * 0.5f,
        pos.y + (kSize - textSize.y) * 0.5f);
    dl->AddText(ImGui::GetFont(), 8.5f, textPos,
        ImGui::ColorConvertFloat4ToU32(c.CMesh), kIcon);
}
