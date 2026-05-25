#include "Panels/EditorChromeTestImGui.h"

#include "Panels/AppHeader.h"
#include "Panels/MainToolbar.h"
#include "Panels/StatusBar.h"

#include "imgui.h"

namespace DeltaEngine
{

DELTAEDITOR_API void EditorChrome_CreateTestImGuiContext(ImVec2 displaySize)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = displaySize;
    ImGui::StyleColorsDark();
    unsigned char* fontTex = nullptr;
    int              w = 0, h = 0;
    io.Fonts->GetTexDataAsRGBA32(&fontTex, &w, &h);
    (void)fontTex;
}

DELTAEDITOR_API void EditorChrome_DestroyTestImGuiContext()
{
    ImGui::DestroyContext();
}

DELTAEDITOR_API bool EditorChrome_Test_DrawAppHeader(const EditorChromeContext& ctx)
{
    DELTA_ASSERT(ImGui::GetCurrentContext() != nullptr);
    ImGui::NewFrame();
    AppHeader header;
    header.Draw(ctx);
    ImGui::Render();
    return true;
}

DELTAEDITOR_API bool EditorChrome_Test_DrawMainToolbar(const EditorChromeContext& ctx)
{
    DELTA_ASSERT(ImGui::GetCurrentContext() != nullptr);
    ImGui::NewFrame();
    MainToolbar toolbar;
    toolbar.Draw(ctx);
    ImGui::Render();
    return true;
}

DELTAEDITOR_API bool EditorChrome_Test_DrawStatusBar(const EditorChromeContext& ctx, const StatusBar* bar)
{
    DELTA_ASSERT(ImGui::GetCurrentContext() != nullptr);
    ImGui::NewFrame();
    StatusBar copy = bar ? *bar : StatusBar{};
    copy.Draw(ctx);
    ImGui::Render();
    return true;
}

}
