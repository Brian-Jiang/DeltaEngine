#pragma once

#include "EditorIncludes.h"

#include "Panels/EditorChromeContext.h"

#include "UIComponents/ClassPickerPopup.h"

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API AppHeader
{
public:
    void Draw();
    void Draw(const EditorChromeContext& ctx);

private:
    ClassPickerPopup m_goPickerPopup;
};

DELTA_ENGINE_NS_END
