#pragma once

#include "EngineIncludes.h"

#include <memory>

DELTA_ENGINE_NS_BEGIN

class GameObject;

class EditorWindow_WorldOutliner
{
public:
    EditorWindow_WorldOutliner();
    ~EditorWindow_WorldOutliner();

    void Render();

    const char* m_title = "World Outliner";
    bool* m_open = nullptr;

private:
    std::shared_ptr<GameObject> m_selectedGameObject;
};

DELTA_ENGINE_NS_END
