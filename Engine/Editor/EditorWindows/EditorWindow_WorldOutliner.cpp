#include "Editor/EditorWindows/EditorWindow_WorldOutliner.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <format>

#include "Editor/EditorCore.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/Style/EditorTheme.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Reflection/DClass.h"

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

    if (!g_editorCore || !g_editorCore->GetEngine())
    {
        ImGui::TextDisabled("No engine");
        ImGui::End();
        return;
    }

    auto world = g_editorCore->GetWorld();
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

    m_selectedIndex = -1;
    if (EditorSelectionState* sel = g_editorCore->GetSelectionState())
    {
        const AssetId sa  = sel->GetSelectedAssetId();
        const ObjectId so = sel->GetSelectedObjectId();
        if (!sa.IsNull() && !so.IsNull())
        {
            for (int i = 0; i < static_cast<int>(gameObjects.size()); ++i)
            {
                GameObject* go = gameObjects[i];
                if (!go)
                    continue;
                auto [a, o] = g_editorCore->GetIdsForObject(go);
                if (a == sa && o == so)
                {
                    m_selectedIndex = i;
                    break;
                }
            }
        }
    }

    RebuildFilter();

    {
        const float btnPadX = 10.f;
        const float btnPadY = 4.f;
        const char* addLabel = "+ Add";
        ImVec2 labelSz = ImGui::CalcTextSize(addLabel);
        float btnW = labelSz.x + btnPadX * 2.f;
        float btnH = labelSz.y + btnPadY * 2.f;

        float availW = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availW - btnW);

        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(c.Acc.x, c.Acc.y, c.Acc.z, 0.12f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(c.Acc.x, c.Acc.y, c.Acc.z, 0.22f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(c.Acc.x, c.Acc.y, c.Acc.z, 0.32f));
        ImGui::PushStyleColor(ImGuiCol_Text,          c.Acc);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(btnPadX, btnPadY));

        if (ImGui::Button(addLabel, ImVec2(btnW, btnH)))
        {
            const DClass* goBaseClass = GetReflectionRegistry().FindClassByName("GameObject");
            if (goBaseClass)
            {
                std::vector<const DClass*> goClasses;
                for (const auto& [name, cls] : GetReflectionRegistry().GetAllClasses())
                {
                    if (cls->IsChildOf(goBaseClass) && !cls->IsAbstract())
                        goClasses.push_back(cls);
                }
                m_addGoPicker.Open(std::move(goClasses));
                ImGui::OpenPopup("##ClassPicker");
            }
        }

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(4);
    }

    if (const DClass* picked = m_addGoPicker.Draw(c))
    {
        const std::string goName = std::format("New {}", picked->GetName());
        const ObjectId newId = g_editorCore->CreateGameObject(goName);
        if (!newId.IsNull())
            if (DPrimaryAsset* asset = g_editorCore->GetActiveSceneAsset())
                g_editorCore->GetSelectionState()->SetSelection(asset->GetAssetId(), newId);
    }

    ImGui::PushStyleColor(ImGuiCol_FrameBg,        c.DInput);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, c.DHover);
    ImGui::PushStyleColor(ImGuiCol_Border,         c.BLight);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,   4.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,    ImVec2(7.f, 10.f));

    ImVec2 filterOrigin = ImGui::GetCursorScreenPos();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputText("##OutlinerFilter", m_filterBuf, sizeof(m_filterBuf));

    if (m_filterBuf[0] == '\0')
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddText(ImVec2(filterOrigin.x + 7.f,  filterOrigin.y + 10.f),
            ImGui::ColorConvertFloat4ToU32(c.TDim), "\xef\x80\x82");
        dl->AddText(ImVec2(filterOrigin.x + 50.f, filterOrigin.y + 10.f),
            ImGui::ColorConvertFloat4ToU32(c.TDim), "Filter objects...");
    }

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(3);

    ImGui::BeginChild("##OutlinerList", ImVec2(0.f, 0.f), ImGuiChildFlags_None,
        ImGuiWindowFlags_NoScrollbar);

    for (const OutlinerEntry* entry : m_filtered)
    {
        ImGui::PushID(entry->index);
        ImVec2 rowMin = ImGui::GetCursorScreenPos();
        ImVec2 rowMax = ImVec2(rowMin.x + ImGui::GetContentRegionAvail().x,
                               rowMin.y + EditorTheme::RowH());
        bool isSelected = (entry->index == m_selectedIndex);

        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, c.DHover);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  c.DHover);

        char rowId[32];
        std::snprintf(rowId, sizeof(rowId), "##olrow%d", entry->index);
        if (ImGui::Selectable(rowId, isSelected,
                ImGuiSelectableFlags_SpanAllColumns, ImVec2(0.f, EditorTheme::RowH())))
        {
            m_selectedIndex = entry->index;
            if (entry->go)
            {
                if (EditorSelectionState* sel = g_editorCore->GetSelectionState())
                {
                    auto [a, o] = g_editorCore->GetIdsForObject(entry->go);
                    if (!a.IsNull() && !o.IsNull())
                        sel->SetSelection(a, o);
                }
            }
        }
        ImGui::PopStyleColor(3);

        if (ImGui::BeginPopupContextItem())
        {
            m_destroyGoMenu.Open({{"Destroy", [&]() {
                if (entry->go && world)
                {
                    const ObjectId oid = entry->go->GetObjectId();
                    world->DestroyGameObject(entry->go);
                    if (g_editorCore)
                        g_editorCore->NotifyObjectDestroyed(oid);
                    if (auto* sel = g_editorCore->GetSelectionState())
                        sel->ClearSelection();
                }
            }}});
            m_destroyGoMenu.Draw(c);
            ImGui::EndPopup();
        }

        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (isSelected)
        {
            dl->AddRectFilledMultiColor(rowMin, rowMax,
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.42f, 0.55f, 1.f, 0.08f)),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.42f, 0.55f, 1.f, 0.04f)),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.42f, 0.55f, 1.f, 0.04f)),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.42f, 0.55f, 1.f, 0.08f)));
            dl->AddLine(rowMin, ImVec2(rowMax.x, rowMin.y),
                ImGui::ColorConvertFloat4ToU32(c.AccRim), 1.f);
            dl->AddLine(ImVec2(rowMin.x, rowMax.y - 1.f),
                        ImVec2(rowMax.x, rowMax.y - 1.f),
                ImGui::ColorConvertFloat4ToU32(c.AccRim), 1.f);
            dl->AddRectFilled(rowMin, ImVec2(rowMin.x + 2.f, rowMax.y),
                ImGui::ColorConvertFloat4ToU32(c.Acc));
        }

        const float textLineH = ImGui::GetTextLineHeight();
        const float contentY  = rowMin.y + (EditorTheme::RowH() - textLineH) * 0.5f;
        ImGui::SetCursorScreenPos(ImVec2(rowMin.x + 5.f, contentY));

        ImFont* monoFont = theme->GetMonoFont();
        if (monoFont) ImGui::PushFont(monoFont);
        ImGui::PushStyleColor(ImGuiCol_Text, c.TGhost);
        ImGui::Text("%d", entry->index);
        ImGui::PopStyleColor();
        if (monoFont) ImGui::PopFont();

        ImGui::SameLine(0.f, 10.f);
        m_typeChip.Draw(c);
        ImGui::SameLine(0.f, 15.f);

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

        ImGui::PopID();
    }

    ImGui::EndChild();
    ImGui::End();
}
