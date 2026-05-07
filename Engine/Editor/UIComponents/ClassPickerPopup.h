#pragma once

#include "EditorIncludes.h"

#include <vector>

#include "Style/EditorTheme.h"

DELTA_ENGINE_NS_BEGIN

class DClass;

class DELTAEDITOR_API ClassPickerPopup
{
public:
    // Replaces the class list and clears the filter; call before OpenPopup("##ClassPicker").
    void Open(std::vector<const DClass*> classes);

    // Returns the picked class when the user confirms a row; otherwise nullptr.
    const DClass* Draw(const EditorTheme::ThemeColors& c);

private:
    void RebuildFilter();

    char                        m_filterBuf[128] = {};
    std::vector<const DClass*>  m_allClasses;
    std::vector<const DClass*>  m_filtered;
    bool                        m_justOpened = false;
};

DELTA_ENGINE_NS_END
