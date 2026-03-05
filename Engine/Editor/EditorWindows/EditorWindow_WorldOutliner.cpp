#include "Editor/EditorWindows/EditorWindow_WorldOutliner.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/Style/EditorTheme.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"

#include "imgui.h"

using namespace DeltaEngine;

EditorWindow_WorldOutliner::EditorWindow_WorldOutliner()
{
}

EditorWindow_WorldOutliner::~EditorWindow_WorldOutliner()
{
}

void EditorWindow_WorldOutliner::RebuildFilter()
{
    m_filtered.clear();
    m_filtered.reserve(m_entries.size());

    if (m_filterBuf[0] == '\0')
    {
        for (const auto& e : m_entries)
            m_filtered.push_back(&e);
        return;
    }

    // Case-insensitive substring match
    auto toLower = [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); };
    std::string needle;
    needle.reserve(std::strlen(m_filterBuf));
    for (const char* p = m_filterBuf; *p; ++p)
        needle += toLower(static_cast<unsigned char>(*p));

    for (const auto& e : m_entries)
    {
        std::string haystack;
        haystack.reserve(e.name.size());
        for (unsigned char ch : e.name)
            haystack += toLower(ch);

        if (haystack.find(needle) != std::string::npos)
            m_filtered.push_back(&e);
    }
}

void EditorWindow_WorldOutliner::Render()
{
    if (!ImGui::Begin(m_title, m_open))
    {
        ImGui::End();
        return;
    }

    if (!g_editor || !g_editor->GetEngine())
    {
        ImGui::TextDisabled("No engine");
        ImGui::End();
        return;
    }

    auto world = g_editor->GetEngine()->GetWorld();
    if (!world)
    {
        ImGui::TextDisabled("No world");
        ImGui::End();
        return;
    }

    EditorTheme* theme = g_editor->GetEditorTheme();
    if (!theme)
    {
        ImGui::End();
        return;
    }
    const EditorTheme::ThemeColors& c = theme->colors;

    // --- Build entries from live game objects ---
    const auto& gameObjects = world->GetGameObjects();
    m_entries.clear();
    m_entries.reserve(gameObjects.size());
    for (int i = 0; i < static_cast<int>(gameObjects.size()); ++i)
    {
        GameObject* go = gameObjects[i];
        OutlinerEntry e;
        e.index   = i;
        e.name    = go ? go->GetName() : "(null)";
        e.visible = true;
        e.go      = go;
        m_entries.push_back(std::move(e));
    }

    // --- Sync selected index from selection state ---
    m_selectedIndex = -1;
    if (auto* sel = g_editor->GetSelectionState())
    {
        const auto& selected = sel->GetSelectedGameObjects();
        for (int i = 0; i < static_cast<int>(gameObjects.size()); ++i)
        {
            for (auto* sgo : selected)
            {
                if (sgo == gameObjects[i])
                {
                    m_selectedIndex = i;
                    break;
                }
            }
            if (m_selectedIndex >= 0)
                break;
        }
    }

    RebuildFilter();

    // --- Filter bar ---
    ImGui::PushStyleColor(ImGuiCol_FrameBg,        c.DInput);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, c.DHover);
    ImGui::PushStyleColor(ImGuiCol_Border,         c.BLight);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,    ImVec2(7.f, 2.f));

    ImVec2 filterOrigin = ImGui::GetCursorScreenPos();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##OutlinerFilter", m_filterBuf, sizeof(m_filterBuf));

    if (m_filterBuf[0] == '\0')
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        // ⌕ search glyph (\xe2\x8c\x95)
        dl->AddText(ImVec2(filterOrigin.x + 7.f,  filterOrigin.y + 3.f),
            ImGui::ColorConvertFloat4ToU32(c.TDim), "\xe2\x8c\x95");
        dl->AddText(ImVec2(filterOrigin.x + 20.f, filterOrigin.y + 3.f),
            ImGui::ColorConvertFloat4ToU32(c.TDim), "Filter objects...");
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);

    // --- Object list ---
    ImGui::BeginChild("##OutlinerList", ImVec2(0.f, 0.f), ImGuiChildFlags_None,
        ImGuiWindowFlags_NoScrollbar);

    for (const OutlinerEntry* entry : m_filtered)
    {
        ImVec2 rowMin = ImGui::GetCursorScreenPos();
        ImVec2 rowMax = ImVec2(rowMin.x + ImGui::GetContentRegionAvail().x,
                               rowMin.y + EditorTheme::RowH());
        bool isSelected = (entry->index == m_selectedIndex);

        // Invisible full-row selectable
        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, c.DHover);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  c.DHover);

        char rowId[32];
        std::snprintf(rowId, sizeof(rowId), "##olrow%d", entry->index);
        if (ImGui::Selectable(rowId, isSelected,
                ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.f, EditorTheme::RowH())))
        {
            m_selectedIndex = entry->index;
            if (auto* sel = g_editor->GetSelectionState())
                sel->SelectGameObject(entry->go);
        }
        bool isHovered = ImGui::IsItemHovered();
        ImGui::PopStyleColor(3);

        // DrawList decorations for selected state
        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (isSelected)
        {
            // Gradient bg tint — periwinkle left-to-right fade
            dl->AddRectFilledMultiColor(rowMin, rowMax,
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.42f, 0.55f, 1.f, 0.08f)),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.42f, 0.55f, 1.f, 0.04f)),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.42f, 0.55f, 1.f, 0.04f)),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.42f, 0.55f, 1.f, 0.08f)));
            // Top AccRim line
            dl->AddLine(rowMin, ImVec2(rowMax.x, rowMin.y),
                ImGui::ColorConvertFloat4ToU32(c.AccRim), 1.f);
            // Bottom AccRim line
            dl->AddLine(ImVec2(rowMin.x, rowMax.y - 1.f),
                        ImVec2(rowMax.x, rowMax.y - 1.f),
                ImGui::ColorConvertFloat4ToU32(c.AccRim), 1.f);
            // 2px left accent bar
            dl->AddRectFilled(rowMin, ImVec2(rowMin.x + 2.f, rowMax.y),
                ImGui::ColorConvertFloat4ToU32(c.Acc));
        }

        // Rewind cursor onto the selectable for row content
        const float textLineH = ImGui::GetTextLineHeight();
        const float contentY  = rowMin.y + (EditorTheme::RowH() - textLineH) * 0.5f;
        ImGui::SetCursorScreenPos(ImVec2(rowMin.x + 5.f, contentY));

        // a) Index — mono font, TGhost color
        ImFont* monoFont = theme->GetMonoFont();
        if (monoFont) ImGui::PushFont(monoFont);
        ImGui::PushStyleColor(ImGuiCol_Text, c.TGhost);
        ImGui::Text("%d", entry->index);
        ImGui::PopStyleColor();
        if (monoFont) ImGui::PopFont();

        // b) Type chip
        ImGui::SameLine(0.f, 10.f);
        m_typeChip.Draw(c);
        ImGui::SameLine(0.f, 15.f);

        // c) Name — clipped, bold+bright when selected
        float nameMaxW = rowMax.x - ImGui::GetCursorScreenPos().x - 21.f;
        ImVec2 nameStart = ImGui::GetCursorScreenPos();
        ImGui::PushClipRect(nameStart,
            ImVec2(nameStart.x + nameMaxW, rowMax.y), true);

        ImFont* boldFont = theme->GetBoldFont();
        if (isSelected && boldFont) ImGui::PushFont(boldFont);
        ImGui::PushStyleColor(ImGuiCol_Text, isSelected ? c.TBright : c.TPrimary);
        ImGui::TextUnformatted(entry->name.c_str());
        ImGui::PopStyleColor();
        if (isSelected && boldFont) ImGui::PopFont();

        ImGui::PopClipRect();

        // d) Visibility dot — right-aligned, shown on hover or selection
        // ● = \xe2\x97\x8f
        if (isHovered || isSelected)
        {
            ImGui::SetCursorScreenPos(
                ImVec2(rowMax.x - 14.f, contentY));
            ImGui::PushStyleColor(ImGuiCol_Text, isSelected ? c.Acc : c.TLabel);
            ImGui::TextUnformatted("\xe2\x97\x8f");
            ImGui::PopStyleColor();
        }
    }

    ImGui::EndChild();
    ImGui::End();
}
