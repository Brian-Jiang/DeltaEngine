#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <vector>

#include "EditorWindows/EditorWindow.h"

DELTA_ENGINE_NS_BEGIN

class GameObject;

class EditorWindow_WorldOutliner : public EditorWindow
{
public:
    EditorWindow_WorldOutliner();
    ~EditorWindow_WorldOutliner();

    void Render() override;

    const char* m_title = "World Outliner";
    bool* m_open = nullptr;

private:
    void RefreshSortedIndices(const std::vector<std::shared_ptr<GameObject>>& gameObjects);

private:
    std::weak_ptr<GameObject> m_selectedGameObject;
    std::vector<int> m_sortedIndices;
    bool m_init = false;
};

DELTA_ENGINE_NS_END
