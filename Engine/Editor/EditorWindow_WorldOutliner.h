#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class EditorWindow_WorldOutliner
{
public:
    EditorWindow_WorldOutliner();
    ~EditorWindow_WorldOutliner();

    void Render();

    const char* m_title = "World Outliner";
    bool* m_open = nullptr;
};

DELTA_ENGINE_NS_END
