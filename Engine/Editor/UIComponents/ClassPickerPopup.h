#pragma once

#include "EngineIncludes.h"

#include <vector>

#include "Style/EditorTheme.h"

DELTA_ENGINE_NS_BEGIN

class DClass;

class ClassPickerPopup
{
public:
    // Call before ImGui::OpenPopup("##ClassPicker") to supply the filtered class list.
    // Resets the search field and requests keyboard focus for the first frame.
    void Open(std::vector<const DClass*> classes);

    // Call every frame after ImGui::OpenPopup("##ClassPicker").
    // Returns a non-null DClass* on the frame the user clicks a row, nullptr otherwise.
    // The popup closes automatically when the user clicks outside.
    const DClass* Draw(const EditorTheme::ThemeColors& c);

private:
    void RebuildFilter();

    char                        m_filterBuf[128] = {};
    std::vector<const DClass*>  m_allClasses;
    std::vector<const DClass*>  m_filtered;
    bool                        m_justOpened = false;
};

DELTA_ENGINE_NS_END
