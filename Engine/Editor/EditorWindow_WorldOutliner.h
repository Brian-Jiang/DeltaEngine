#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <vector>

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
    void RefreshSortedIndices(const std::vector<std::shared_ptr<GameObject>>& gameObjects);

private:
    std::shared_ptr<GameObject> m_selectedGameObject;
    std::vector<int> m_sortedIndices;
    bool m_init = false;
    //bool m_sortedIndicesDirty = true;
};

DELTA_ENGINE_NS_END
