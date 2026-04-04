#include "Panels/AppHeader.h"

#include "Commands/EditorAuxiliarySceneCommands.h"
#include "Commands/EditorCommand_CreateGameObject.h"
#include "Commands/EditorCommandContext.h"
#include "Commands/EditorCommandManager.h"
#include "EditorCore.h"
#include "EditorMain.h"
#include "Assets/EditorAssetDatabase.h"
#include "Style/EditorTheme.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Reflection/DClass.h"

#include "imgui.h"

using namespace DeltaEngine;

void AppHeader::Draw()
{
    ImGuiIO& io = ImGui::GetIO();
    EditorTheme* theme = g_editor->GetEditorTheme();
    const auto& c = theme->colors;

    const float fh = ImGui::GetFrameHeight();
    const float fs = ImGui::GetFontSize();
    const float pad = ImGui::GetStyle().ItemSpacing.x;

    {
        const auto& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,
            ImVec2(style.FramePadding.x, style.FramePadding.y + fs * 0.25f));
    }
    const float hdrH = ImGui::GetFrameHeight();

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, hdrH));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, c.DFloor);
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, c.DFloor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;

    ImGui::Begin("##AppHeader", nullptr, flags);
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetCursorScreenPos();

    float sepY = winPos.y + hdrH - 1.f;
    dl->AddLine(
        ImVec2(winPos.x, sepY),
        ImVec2(winPos.x + io.DisplaySize.x, sepY),
        ImGui::ColorConvertFloat4ToU32(c.BDeep), 1.f);

    if (ImGui::BeginMenuBar()) {
        {
            const float side = fs;
            const float triH = side * 0.866f;
            ImVec2 sp = ImGui::GetCursorScreenPos();
            float offY = 13.0f;
            ImVec2 p1(sp.x, sp.y + offY + triH);
            ImVec2 p2(sp.x + side, sp.y + offY + triH);
            ImVec2 p3(sp.x + side * 0.5f, sp.y + offY);
            dl->AddTriangleFilled(p1, p2, p3, ImGui::ColorConvertFloat4ToU32(c.AccHi));
            ImGui::Dummy(ImVec2(side, 0.f));
        }

        ImGui::SameLine(0.f, pad * 1.0f);
        if (theme->GetBoldFont())
            ImGui::PushFont(theme->GetBoldFont());
        ImGui::Text("Delta Engine");
        if (theme->GetBoldFont())
            ImGui::PopFont();

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                if (g_editorCore)
                {
                    EditorCommandContext ctx{ *g_editorCore };
                    g_editorCore->GetCommandManager().ExecuteAuxiliary(
                        std::make_unique<EditorAuxiliaryCommand_SaveScene>(), ctx);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            auto& cmdMgr = g_editorCore->GetCommandManager();
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, cmdMgr.CanUndo()))
            {
                EditorCommandContext ctx{ *g_editorCore };
                cmdMgr.Undo(ctx);
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, cmdMgr.CanRedo()))
            {
                EditorCommandContext ctx{ *g_editorCore };
                cmdMgr.Redo(ctx);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
            ImGui::EndMenu();
        if (ImGui::BeginMenu("Scene"))
            ImGui::EndMenu();
        if (ImGui::BeginMenu("Object"))
        {
            if (ImGui::MenuItem("Add GameObject..."))
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
                    m_goPickerPopup.Open(std::move(goClasses));
                    ImGui::OpenPopup("##ClassPicker");
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Build"))
            ImGui::EndMenu();
        if (ImGui::BeginMenu("Help"))
            ImGui::EndMenu();

        const float btnW = fs * 4.5f;
        const float iconW = fh;
        const float groupW = btnW + pad * 0.5f + iconW + pad;

        ImGui::SameLine(io.DisplaySize.x - groupW);

        ImGui::PushStyleColor(ImGuiCol_Text, c.Ok);
        ImGui::PushStyleColor(ImGuiCol_Border, c.BLight);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.f);
        ImGui::Button("Debug x64", ImVec2(0.f, 0.f));
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        ImGui::SameLine(0.f, pad * 0.5f);
        if (ImGui::Button("⚙##Settings", ImVec2(0.f, 0.f)))
        { /* placeholder */
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Settings");

        ImGui::EndMenuBar();
    }

    if (const DClass* picked = m_goPickerPopup.Draw(c))
    {
        if (g_editorCore)
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
    }

    ImGui::PopStyleVar();
    ImGui::End();
}
