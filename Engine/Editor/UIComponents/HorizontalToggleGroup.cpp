#include "UIComponents/HorizontalToggleGroup.h"

#include "UIComponents/UIComponentsEditorTheme.h"

#include "imgui.h"

using namespace DeltaEngine;

WidgetEditEvent HorizontalToggleGroup::Draw(const char* id, const EditorTheme::ThemeColors& colors, const Item* items,
                                            int itemCount, int& selected, float itemW, float itemH,
                                            const int* overrideSelectedIndex, const ImVec4* overrideSelectedColor)
{
    if (!id)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "HorizontalToggleGroup::Draw: id was null (expected non-null ImGui ID string)");
        return {};
    }
    if (!items || itemCount <= 0)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "HorizontalToggleGroup::Draw: expected non-null items and positive itemCount (items={}, itemCount={})",
            static_cast<const void*>(items), itemCount);
        return {};
    }

    const auto& c          = colors;
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (!drawList)
    {
        DLOG(LogUIComponents, ELogLevel::Warning,
            "HorizontalToggleGroup::Draw: ImGui::GetWindowDrawList() returned null (expected active window)");
        return {};
    }

    const float fh = ImGui::GetFrameHeight();
    if (itemH <= 0.f)
        itemH = fh;
    if (itemW <= 0.f)
        itemW = fh;

    const float spacing        = 1.f;
    const float totalW         = itemCount * itemW + (itemCount - 1) * spacing;
    const float rounding       = 6.f;
    const float buttonRounding = 4.f;

    ImGui::PushID(id);
    ImGui::BeginGroup();
    ImVec2 cursorScreen = ImGui::GetCursorScreenPos();
    ImVec2 groupMin     = cursorScreen;
    ImVec2 groupMax     = ImVec2(cursorScreen.x + totalW, cursorScreen.y + itemH);

    drawList->AddRectFilled(groupMin, groupMax, ImGui::ColorConvertFloat4ToU32(c.DRaised), rounding);
    drawList->AddRect(groupMin, groupMax, ImGui::ColorConvertFloat4ToU32(c.BLight), rounding, 0, 1.f);

    bool selectionChanged = false;
    for (int i = 0; i < itemCount; ++i)
    {
        ImGui::PushID(i);
        ImVec2 btnMin     = ImGui::GetCursorScreenPos();
        ImVec2 btnSize(itemW, itemH);
        ImGui::InvisibleButton("##btn", btnSize);
        bool hovered      = ImGui::IsItemHovered();
        bool clicked      = ImGui::IsItemClicked();
        bool isSelected   = (selected == i);

        if (clicked && !isSelected)
        {
            selected           = i;
            selectionChanged = true;
        }

        ImVec4 bgColor     = ImVec4(0, 0, 0, 0);
        ImVec4 textColor   = c.TLabel;
        bool drawBorder    = false;
        ImVec4 borderColor = c.AccMid;

        if (isSelected)
        {
            bgColor = c.AccBg;
            bool useOverride = (overrideSelectedIndex && overrideSelectedColor && *overrideSelectedIndex == i);
            if (useOverride)
            {
                textColor    = *overrideSelectedColor;
                borderColor = *overrideSelectedColor;
            }
            else
            {
                textColor    = c.AccHi;
                borderColor = c.AccMid;
            }
            drawBorder = true;
        }
        else if (hovered)
        {
            bgColor   = c.DHover;
            textColor = c.TPrimary;
        }

        if (bgColor.w > 0.001f)
        {
            ImVec2 rectMin = btnMin;
            ImVec2 rectMax = ImVec2(btnMin.x + itemW, btnMin.y + itemH);
            drawList->AddRectFilled(rectMin, rectMax, ImGui::ColorConvertFloat4ToU32(bgColor), buttonRounding);
        }
        if (drawBorder)
        {
            ImVec2 rectMin = btnMin;
            ImVec2 rectMax = ImVec2(btnMin.x + itemW, btnMin.y + itemH);
            drawList->AddRect(rectMin, rectMax, ImGui::ColorConvertFloat4ToU32(borderColor), buttonRounding, 0, 1.f);
        }

        const char* label = items[i].label ? items[i].label : "";
        ImVec2 textSize   = ImGui::CalcTextSize(label);
        ImVec2 textPos    = ImVec2(
            btnMin.x + (itemW - textSize.x) * 0.5f,
            btnMin.y + (itemH - textSize.y) * 0.5f);
        drawList->AddText(textPos, ImGui::ColorConvertFloat4ToU32(textColor), label);

        if (hovered && items[i].tooltip && items[i].tooltip[0] != '\0')
            ImGui::SetTooltip("%s", items[i].tooltip);

        ImGui::PopID();

        if (i < itemCount - 1)
            ImGui::SameLine(0.f, spacing);
    }

    ImGui::EndGroup();
    ImGui::PopID();

    if (selectionChanged)
    {
        WidgetEditEvent evt;
        evt.valueChanged = true;
        evt.editBegan    = true;
        evt.editEnded    = true;
        return evt;
    }
    return {};
}
