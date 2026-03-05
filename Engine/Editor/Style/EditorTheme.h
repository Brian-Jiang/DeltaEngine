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
        ImVec4 DInput;   // 0x0b0f1a — input field bg (between DFloor and DRaised)
        ImVec4 TBright;  // 0xecf0ff — brightest text (object name in header)
        ImVec4 Err;      // 0xef4444 — error/invalid value highlight
        ImVec4 AxX;      // 0xf87171 — X axis label text (red)
        ImVec4 AxY;      // 0x4ade80 — Y axis label text (green)
        ImVec4 AxZ;      // 0x60a5fa — Z axis label text (blue)
        ImVec4 AxXBg;    // 0x3b1212 — X axis chip background (dark red)
        ImVec4 AxYBg;    // 0x0f2e18 — Y axis chip background (dark green)
        ImVec4 AxZBg;    // 0x0d1e38 — Z axis chip background (dark blue)
        ImVec4 Acc;      // 0x6B8CFF @ 0.85 — main periwinkle (left bar, selected dot)
        ImVec4 AccRim;   // 0x6B8CFF @ 0.18 — rim lines (selection top/bottom)
    };

    //static constexpr float kRowH = 30.f;
    static float RowH()
    {
        return ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f;
    }

    static float HdrH()
    {
        const float kExtra = ImGui::GetFontSize() * 0.25f;
        return ImGui::GetFontSize()
            + (ImGui::GetStyle().FramePadding.y + kExtra) * 2.f;
    }

    static float TbH()
    {
        const float kItemH = ImGui::GetFontSize() * 2.0f;
        return kItemH + ImGui::GetStyle().ItemSpacing.y * 2.f;
    }

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
    ImFont* TryLoadFont(std::string path, bool withFaSolid);
    ImVec4 HexToVec4(uint32_t hex, float alphaOverride = -1.f) const;

    ImFont* m_regularFont;
    ImFont* m_boldFont;
    ImFont* m_monoFont;
    ImFont* m_faSolidFont;
};

DELTA_ENGINE_NS_END
