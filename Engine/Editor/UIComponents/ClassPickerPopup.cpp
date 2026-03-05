#include "UIComponents/ClassPickerPopup.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "Runtime/Reflection/DClass.h"
#include "Style/EditorTheme.h"

#include "imgui.h"
#include "imgui_internal.h"

using namespace DeltaEngine;

void ClassPickerPopup::Open(std::vector<const DClass*> classes)
{
    m_allClasses   = std::move(classes);
    m_filterBuf[0] = '\0';
    m_justOpened   = true;
    RebuildFilter();
}

void ClassPickerPopup::RebuildFilter()
{
    m_filtered.clear();
    m_filtered.reserve(m_allClasses.size());

    if (m_filterBuf[0] == '\0')
    {
        for (const DClass* cls : m_allClasses)
            m_filtered.push_back(cls);
        return;
    }

    auto toLower = [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); };

    std::string needle;
    needle.reserve(std::strlen(m_filterBuf));
    for (const char* p = m_filterBuf; *p; ++p)
        needle += toLower(static_cast<unsigned char>(*p));

    for (const DClass* cls : m_allClasses)
    {
        std::string haystack;
        const std::string& name = cls->GetName();
        haystack.reserve(name.size());
        for (unsigned char ch : name)
            haystack += toLower(ch);

        if (haystack.find(needle) != std::string::npos)
            m_filtered.push_back(cls);
    }
}

const DClass* ClassPickerPopup::Draw(const EditorTheme::ThemeColors& c)
{
    if (!ImGui::BeginPopup("##ClassPicker"))
        return nullptr;

    const DClass* picked = nullptr;

    // ── Search bar ───────────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        c.DInput);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, c.DHover);
    ImGui::PushStyleColor(ImGuiCol_Border,         c.BLight);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,    ImVec2(7.f, 6.f));

    if (m_justOpened)
        ImGui::SetKeyboardFocusHere();

    ImVec2 filterOrigin = ImGui::GetCursorScreenPos();
    ImGui::SetNextItemWidth(240.f);
    bool filterChanged = ImGui::InputText("##CPFilter", m_filterBuf, sizeof(m_filterBuf));

    if (m_filterBuf[0] == '\0')
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddText(ImVec2(filterOrigin.x + 7.f, filterOrigin.y + 6.f),
            ImGui::ColorConvertFloat4ToU32(c.TDim), "Search...");
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);

    m_justOpened = false;

    if (filterChanged)
        RebuildFilter();

    // ── Scrollable class list ─────────────────────────────────────────────────
    ImGui::BeginChild("##CPList", ImVec2(240.f, 280.f), ImGuiChildFlags_None,
        ImGuiWindowFlags_NoScrollbar);

    constexpr const char* kIcon    = "\xef\x86\xb2"; // FA cube U+F1B2
    constexpr float       kIconSz  = 14.f;

    for (const DClass* cls : m_filtered)
    {
        ImVec2 rowMin = ImGui::GetCursorScreenPos();
        float  rowH   = EditorTheme::RowH();
        float  rowW   = ImGui::GetContentRegionAvail().x;

        // Invisible full-row hit area
        char btnId[64];
        std::snprintf(btnId, sizeof(btnId), "##cprow_%p", static_cast<const void*>(cls));
        ImGui::InvisibleButton(btnId, ImVec2(rowW, rowH));
        bool hovered = ImGui::IsItemHovered();
        bool clicked = ImGui::IsItemClicked();

        ImDrawList* dl = ImGui::GetWindowDrawList();

        if (hovered)
        {
            dl->AddRectFilled(rowMin,
                ImVec2(rowMin.x + rowW, rowMin.y + rowH),
                ImGui::ColorConvertFloat4ToU32(c.DHover));
        }

        // Icon — vertically centered
        const float textLineH = ImGui::GetTextLineHeight();
        float contentY = rowMin.y + (rowH - textLineH) * 0.5f;
        float iconOffY = (textLineH - kIconSz) * 0.5f;

        ImVec2 iconPos(rowMin.x + 8.f, contentY + iconOffY);
        dl->AddText(ImGui::GetFont(), kIconSz, iconPos,
            ImGui::ColorConvertFloat4ToU32(c.CMesh), kIcon);

        // Class name — vertically centered, left of icon
        ImVec2 textPos(iconPos.x + kIconSz + 6.f, contentY);
        dl->AddText(textPos,
            ImGui::ColorConvertFloat4ToU32(c.TPrimary),
            cls->GetName().c_str());

        if (clicked)
            picked = cls;
    }

    ImGui::EndChild();
    ImGui::EndPopup();

    return picked;
}
