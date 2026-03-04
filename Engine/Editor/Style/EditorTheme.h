#pragma once

#include "EngineIncludes.h"

#include <string>

#include "imgui.h"

DELTA_ENGINE_NS_BEGIN

class EditorTheme
{
public:
    EditorTheme();
    ~EditorTheme();

    void ApplyTheme();
    
    ImFont* GetRegularFont() const { return m_regularFont; }
    ImFont* GetBoldFont() const { return m_boldFont; }
    ImFont* GetMonoFont() const { return m_monoFont; }

private:
    void LoadFonts();
    ImFont* TryLoadFont(std::string path);
    ImVec4 HexToVec4(uint32_t hex, float alphaOverride = -1.f) const;

private:
    ImFont* m_regularFont;
    ImFont* m_boldFont;
    ImFont* m_monoFont;
};

//void ApplyDeltaTheme();

// Font handles — nullptr if load failed; callers must guard with if (GFontMono) ImGui::PushFont(GFontMono)
//extern ImFont* GFontUI;      // Outfit Regular 12px — all labels
//extern ImFont* GFontUIBold;  // Outfit Bold 12px — section headers, object name
//extern ImFont* GFontMono;    // JetBrains Mono 10.5px — all numeric fields

// Size constants for layout (Periwinkle Night v5)
//constexpr float kRowH = 22.f;   // property row height
//constexpr float kSecH = 23.f;   // inspector section header height
//constexpr float kPtbH = 25.f;   // panel titlebar height
//constexpr float kHdrH = 30.f;   // app header height
//constexpr float kTbH  = 34.f;   // toolbar height
//constexpr float kStH  = 20.f;   // status bar height


DELTA_ENGINE_NS_END
