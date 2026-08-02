#include "Editor/EditorWindows/EditorWindow_WorldOutliner.h"

#include <cstdio>
#include <vector>

#include "Editor/Commands/EditorCommand_CreateGameObject.h"
#include "Editor/Commands/EditorCommand_DeleteGameObject.h"
#include "Editor/Commands/EditorCommand_RenameObject.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/EditorWindows/EditorOutlinerFiltering.h"
#include "Editor/EditorWindows/EditorWindowsLog.h"
#include "Editor/Style/EditorTheme.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Reflection/DClass.h"

#include "imgui.h"

#include <vector>

using namespace DeltaEngine;

EditorWindow_WorldOutliner::EditorWindow_WorldOutliner()
{
    m_title = "World Outliner";

    if (g_editorCore)
    {
        if (EditorSelectionState* sel = g_editorCore->GetSelectionState())
            m_onSelectionChangedHandle = sel->OnSelectionChanged.AddRaw(this, &EditorWindow_WorldOutliner::HandleSelectionChanged);
    }
}

EditorWindow_WorldOutliner::~EditorWindow_WorldOutliner()
{
    if (g_editorCore)
    {
        if (EditorSelectionState* sel = g_editorCore->GetSelectionState())
            sel->OnSelectionChanged.Remove(m_onSelectionChangedHandle);
    }
}

void EditorWindow_WorldOutliner::HandleSelectionChanged()
{
    ++m_selectionRevision;
    RebuildFilter();
}

void EditorWindow_WorldOutliner::RebuildFilter()
{
    BuildOutlinerFilterMatches(m_entries, m_filterBuf, m_filtered);
}

void EditorWindow_WorldOutliner::Render(bool& open)
{
    if (!ImGui::Begin(GetImGuiTitle(), &open))
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

    EditorTheme* theme = g_editor ? g_editor->GetEditorTheme() : nullptr;
    if (!theme)
    {
        DLOG(LogEditorWindows, ELogLevel::Warning,
            "WorldOutliner skipped draw: EditorTheme missing from EditorMain (expected initialized theme)");
        ImGui::TextDisabled("Theme unavailable");
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
        DPrimaryAsset* sceneAsset = g_editorCore->GetActiveSceneAsset();
        if (sceneAsset)
        {
            EditorCommandContext ctx{ *g_editorCore };
            g_editorCore->GetCommandManager().Execute(
                std::make_unique<EditorCommand_CreateGameObject>(
                    sceneAsset->GetAssetId(), std::string(picked->GetName())), ctx);
        }
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
        EditorSelectionState* sel = g_editorCore->GetSelectionState();
        bool isSelected = sel && entry->go && sel->IsGameObjectSelected(entry->go->GetObjectId());

        const bool renamingRow =
            m_inlineRename.IsActive() && entry->go && entry->go->GetObjectId() == m_renameObjectId;

        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, c.DHover);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive,  c.DHover);

        char rowId[32];
        std::snprintf(rowId, sizeof(rowId), "##olrow%d", entry->index);
        ImGuiSelectableFlags selFlags = ImGuiSelectableFlags_SpanAllColumns;
        if (renamingRow)
            selFlags |= ImGuiSelectableFlags_AllowOverlap;
        if (ImGui::Selectable(rowId, isSelected, selFlags, ImVec2(0.f, EditorTheme::RowH())))
        {
            if (!renamingRow && entry->go && sel)
            {
                const ObjectId id = entry->go->GetObjectId();
                if (ImGui::GetIO().KeyCtrl)
                {
                    if (sel->IsGameObjectSelected(id))
                        sel->RemoveSelectedGameObject(id);
                    else
                        sel->AddSelectedGameObject(id);
                }
                else
                {
                    sel->SetSelectedGameObject(id);
                }
            }
        }
        ImGui::PopStyleColor(3);

        if (ImGui::BeginPopupContextItem())
        {
            std::vector<ContextMenuPopup::Item> items;
            const bool canRename = sel && entry->go && sel->GetSelectedGameObjects().size() == 1 &&
                sel->GetSelectedGameObjects()[0] == entry->go->GetObjectId();
            if (canRename)
            {
                items.push_back({"Rename", [&]() {
                    if (!entry->go || !g_editorCore)
                        return;
                    m_renameObjectId = entry->go->GetObjectId();
                    m_inlineRename.Begin(entry->go->GetName());
                }});
            }
            items.push_back({"Destroy", [&]() {
                if (entry->go && g_editorCore)
                {
                    auto [assetId, objId] = g_editorCore->GetIdsForObject(entry->go);
                    if (!assetId.IsNull() && !objId.IsNull())
                    {
                        EditorCommandContext ctx{ *g_editorCore };
                        g_editorCore->GetCommandManager().Execute(
                            std::make_unique<EditorCommand_DeleteGameObject>(assetId, objId), ctx);
                    }
                }
            }});
            m_destroyGoMenu.Open(std::move(items));
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

        m_typeChip.Draw(c);
        ImGui::SameLine(0.f, 15.f);

        float nameMaxW = rowMax.x - ImGui::GetCursorScreenPos().x - 21.f;
        ImVec2 nameStart = ImGui::GetCursorScreenPos();
        ImGui::PushClipRect(nameStart,
            ImVec2(nameStart.x + nameMaxW, rowMax.y), true);

        if (renamingRow)
        {
            ImGui::SetNextItemWidth(nameMaxW);
            const auto rr = m_inlineRename.Draw();
            if (rr == EditorInlineRename::Result::Committed && g_editorCore && entry->go)
            {
                auto [assetId, objId] = g_editorCore->GetIdsForObject(entry->go);
                if (!assetId.IsNull() && !objId.IsNull())
                {
                    EditorCommandContext ctx{ *g_editorCore };
                    g_editorCore->GetCommandManager().Execute(
                        std::make_unique<EditorCommand_RenameObject>(
                            assetId, objId, std::string(m_inlineRename.GetBuffer())),
                        ctx);
                }
                m_renameObjectId = ObjectId::Null();
            }
            else if (rr == EditorInlineRename::Result::Cancelled)
                m_renameObjectId = ObjectId::Null();
        }
        else
        {
            ImFont* boldFont = theme->GetBoldFont();
            if (isSelected && boldFont) ImGui::PushFont(boldFont);
            ImGui::PushStyleColor(ImGuiCol_Text, isSelected ? c.TBright : c.TPrimary);
            ImGui::TextUnformatted(entry->name.c_str());
            ImGui::PopStyleColor();
            if (isSelected && boldFont) ImGui::PopFont();
        }

        ImGui::PopClipRect();

        ImGui::PopID();
    }

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::GetIO().WantTextInput &&
        ImGui::IsKeyPressed(ImGuiKey_F2))
    {
        EditorSelectionState* selF2 = g_editorCore->GetSelectionState();
        if (selF2 && selF2->GetSelectedGameObjects().size() == 1)
        {
            const ObjectId id = selF2->GetSelectedGameObjects()[0];
            for (const OutlinerEntry* e : m_filtered)
            {
                if (e->go && e->go->GetObjectId() == id)
                {
                    m_renameObjectId = id;
                    m_inlineRename.Begin(e->go->GetName());
                    break;
                }
            }
        }
    }

    ImGui::EndChild();
    ImGui::End();
}
