#pragma once

#include "EngineIncludes.h"

#include <string>

#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class EditorTheme
{
public:
    struct ThemeColors
    {
        ImVec4 DFloor;   // 0x090c14 — deepest bg (chrome panels)
        ImVec4 DRaised;  // 0x121620 — raised surface (toggle group bg, widget bg)
        ImVec4 DHover;   // 0x1c2234 — hovered surface
        ImVec4 AccBg;    // 0x6B8CFF @ 0.05 — accent bg tint (selected toggle)
        ImVec4 AccMid;   // 0x6B8CFF @ 0.40 — accent mid (selected border)
        ImVec4 AccHi;    // 0x8FAAFF — accent bright (selected text, logo)
        ImVec4 BDeep;    // 0x0a0d16 — deepest border (separator lines)
        ImVec4 BMid;     // 0x191f30 — medium border (dividers)
        ImVec4 BLight;   // 0x222d44 — light border (widget outlines)
        ImVec4 BHover;   // 0x2e3d5e — hover border (hovered input/button outline)
        ImVec4 TPrimary; // 0xbcc4de — primary text
        ImVec4 TLabel;   // 0x6c7898 — label text
        ImVec4 TDim;     // 0x384060 — dim text (status bar labels)
        ImVec4 TGhost;   // 0x1e2640 — ghost text (faintest)
        ImVec4 Ok;       // 0x34d399 — green status/success
        ImVec4 CMesh;    // 0x1ec8b4 — cyan numeric
        ImVec4 DPanel;   // 0x0d1019 — panel body bg (dockable window background)
    };

    static float HdrH() { return ImGui::GetFrameHeight() * 1.5f; }         // 1 row
    static float TbH()  { return ImGui::GetFrameHeight() * 2.0f; }
    static float StH()  { return ImGui::GetFrameHeight(); }         // 1 row

    EditorTheme();
    ~EditorTheme();

    void ApplyTheme();

    ImFont* GetRegularFont() const { return m_regularFont; }
    ImFont* GetBoldFont() const { return m_boldFont; }
    ImFont* GetMonoFont() const { return m_monoFont; }

    ThemeColors colors;

private:
    void LoadFonts();
    ImFont* TryLoadFont(std::string path);
    ImVec4 HexToVec4(uint32_t hex, float alphaOverride = -1.f) const;

    ImFont* m_regularFont;
    ImFont* m_boldFont;
    ImFont* m_monoFont;
};

DELTA_ENGINE_NS_END
