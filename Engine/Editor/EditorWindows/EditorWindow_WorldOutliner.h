#pragma once

#include "UIComponents/ClassPickerPopup.h"
#include "UIComponents/ContextMenuPopup.h"

#include "EngineIncludes.h"

#include <string>
#include <vector>

#include "EditorWindows/EditorWindow.h"
#include "UIComponents/TypeChip.h"

DELTA_ENGINE_NS_BEGIN

class GameObject;

struct OutlinerEntry
{
    /// Index in the world's game object list for this row.
    int index = 0;
    /// Display name (or placeholder if the pointer is null).
    std::string name;
    /// Reserved for future visibility UI.
    bool visible = true;
    /// Game object this row represents; may be null.
    GameObject* go = nullptr;
};

class EditorWindow_WorldOutliner : public EditorWindow
{
public:
    EditorWindow_WorldOutliner();
    ~EditorWindow_WorldOutliner();

    /// Lists world objects, filter, add/destroy via context actions.
    void Render() override;

    /// ImGui window title.
    const char* m_title = "World Outliner";
    /// Optional open flag for ImGui::Begin.
    bool* m_open = nullptr;

private:
    void RebuildFilter();

    std::vector<OutlinerEntry>        m_entries;
    std::vector<const OutlinerEntry*> m_filtered;
    int                               m_selectedIndex = -1;
    char                              m_filterBuf[128] = {};
    TypeChip                          m_typeChip;
    ClassPickerPopup                  m_addGoPicker;
    ContextMenuPopup                  m_destroyGoMenu;
};

DELTA_ENGINE_NS_END
