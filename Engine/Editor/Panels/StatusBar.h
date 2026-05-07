#pragma once

#include "EditorIncludes.h"

#include "Panels/EditorChromeContext.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API StatusBar
{
public:
    // Status text sources; set from scene/editor code before Draw().
    std::string sceneName = "Untitled";
    int objectCount = 0;
    std::string selectedName = "";
    std::string rendererName = "DirectX 12";
    std::string buildConfig = "Debug x64";
    std::string engineVersion = "DeltaEngine 0.1.0";

    void Draw();
    void Draw(const EditorChromeContext& ctx);
};

DELTA_ENGINE_NS_END
