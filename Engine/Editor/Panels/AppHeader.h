#pragma once

#include "EngineIncludes.h"

#include "UIComponents/ClassPickerPopup.h"

DELTA_ENGINE_NS_BEGIN

class AppHeader
{
public:
    // Top menu bar, logo, and class picker for new GameObjects.
    void Draw();

private:
    ClassPickerPopup m_goPickerPopup;
};

DELTA_ENGINE_NS_END
