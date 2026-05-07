#include "Panels/StatusBar.h"

#include "EditorCore.h"
#include "EditorMain.h"
#include "Panels/EditorChromeContext.h"
#include "Style/EditorTheme.h"

#include "imgui.h"

#include <format>
#include <string>

using namespace DeltaEngine;

void StatusBar::Draw()
{
    EditorChromeContext ctx;
    ctx.theme  = g_editor ? g_editor->GetEditorTheme() : nullptr;
    ctx.core   = g_editorCore;
    ctx.editor = g_editor;
    Draw(ctx);
}

void StatusBar::Draw(const EditorChromeContext& ctx)
{
    if (!ctx.theme)
    {
        DLOG(LogEditorChrome, ELogLevel::Warning,
            "StatusBar::Draw skipped: missing EditorTheme (expected non-null ctx.theme; core={}, editor={})",
            static_cast<const void*>(ctx.core), static_cast<const void*>(ctx.editor));
        return;
    }

    EditorTheme* theme = ctx.theme;
    DELTA_ASSERT(theme != nullptr);
    const auto& c = theme->colors;

    ImGuiIO& io = ImGui::GetIO();

    const float fs  = ImGui::GetFontSize();
    const float pad = ImGui::GetStyle().ItemSpacing.x;

    ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - EditorTheme::StH()));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, EditorTheme::StH()));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, c.DFloor);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("##StatusBar", nullptr, flags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 winPos = ImGui::GetWindowPos();
    drawList->AddLine(
        ImVec2(winPos.x, winPos.y),
        ImVec2(winPos.x + io.DisplaySize.x, winPos.y),
        ImGui::ColorConvertFloat4ToU32(c.BDeep), 1.f);

    ImGui::SetCursorPosY((EditorTheme::StH() - fs) * 0.5f);
    ImGui::SetCursorPosX(pad);

    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);

    ImVec2 dotPos = ImGui::GetCursorScreenPos();
    drawList->AddCircleFilled(ImVec2(dotPos.x + fs * 0.2f, dotPos.y + ImGui::GetTextLineHeight() * 0.5f),
        fs * 0.2f, ImGui::ColorConvertFloat4ToU32(c.Ok));
    ImGui::Dummy(ImVec2(fs * 0.4f, 0.f));
    ImGui::SameLine(0, pad * 0.5f);

    ImGui::PopStyleColor();
    if (theme->GetBoldFont())
        ImGui::PushFont(theme->GetBoldFont());
    ImGui::PushStyleColor(ImGuiCol_Text, c.TLabel);
    ImGui::Text("Ready");
    ImGui::PopStyleColor();
    if (theme->GetBoldFont())
        ImGui::PopFont();

    ImGui::SameLine(0, pad);
    ImVec2 divPos = ImGui::GetCursorScreenPos();
    float divY = divPos.y + ImGui::GetTextLineHeight() * 0.5f;
    drawList->AddLine(ImVec2(divPos.x, divY - fs * 0.4f), ImVec2(divPos.x, divY + fs * 0.4f),
        ImGui::ColorConvertFloat4ToU32(c.BMid), 1.f);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 1.f + pad * 0.5f);
    ImGui::SameLine(0, pad * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Scene");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, pad * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TLabel);
    ImGui::Text("%s", sceneName.c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine(0, pad);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Objects");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, pad * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TLabel);
    ImGui::Text("%d", objectCount);
    ImGui::PopStyleColor();

    if (!selectedName.empty())
    {
        ImGui::SameLine(0, pad);
        ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
        ImGui::Text("Selected");
        ImGui::PopStyleColor();
        ImGui::SameLine(0, pad * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, c.AccHi);
        ImGui::Text("%s", selectedName.c_str());
        ImGui::PopStyleColor();
    }

    std::string rightSummary = std::format(
        "Renderer {}  Build {}  {}", rendererName, buildConfig, engineVersion);
    constexpr std::size_t kRightSummarySoftCap = 4096;
    if (rightSummary.size() > kRightSummarySoftCap)
    {
        DLOG(LogEditorChrome, ELogLevel::Warning,
            "StatusBar right summary truncated (got {} chars, soft cap {}): rendererLen={}, buildLen={}, engineLen={}",
            rightSummary.size(), kRightSummarySoftCap,
            rendererName.size(), buildConfig.size(), engineVersion.size());
        rightSummary.resize(kRightSummarySoftCap);
    }

    const float rightW = ImGui::CalcTextSize(rightSummary.c_str()).x + pad * 4.f;

    ImGui::SetCursorPosX(io.DisplaySize.x - rightW - pad);
    ImGui::SetCursorPosY((EditorTheme::StH() - fs) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Renderer");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, pad * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TLabel);
    ImGui::Text("%s", rendererName.c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine(0, pad);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TDim);
    ImGui::Text("Build");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, pad * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Text, c.Ok);
    ImGui::Text("%s", buildConfig.c_str());
    ImGui::PopStyleColor();

    ImGui::SameLine(0, pad);
    ImGui::PushStyleColor(ImGuiCol_Text, c.TGhost);
    ImGui::Text("%s", engineVersion.c_str());
    ImGui::PopStyleColor();

    ImGui::End();
}
