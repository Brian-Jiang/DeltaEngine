#pragma once

#include "UIComponents/ClassPickerPopup.h"

#include "EngineIncludes.h"

#include <string>
#include <vector>

#include "EditorWindows/EditorWindow.h"
#include "UIComponents/TypeChip.h"

DELTA_ENGINE_NS_BEGIN

class GameObject;

struct OutlinerEntry
{
    int          index   = 0;
    std::string  name;
    bool         visible = true;
    GameObject*  go      = nullptr;
};

class EditorWindow_WorldOutliner : public EditorWindow
{
public:
    EditorWindow_WorldOutliner();
    ~EditorWindow_WorldOutliner();

    void Render() override;

    const char* m_title = "World Outliner";
    bool* m_open = nullptr;

private:
    void RebuildFilter();

private:
    std::vector<OutlinerEntry>        m_entries;
    std::vector<const OutlinerEntry*> m_filtered;
    int                               m_selectedIndex = -1;
    char                              m_filterBuf[128] = {};
    TypeChip                          m_typeChip;
    ClassPickerPopup                  m_addGoPicker;
};

DELTA_ENGINE_NS_END
