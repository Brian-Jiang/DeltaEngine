#pragma once

#include "EngineIncludes.h"

#include <string>

DELTA_ENGINE_NS_BEGIN

class StatusBar
{
public:
    // Populated each frame by whoever owns scene state.
    // Left as plain strings for now — replace with real data later.
    std::string sceneName = "Untitled";
    int objectCount = 0;
    std::string selectedName = "";
    std::string rendererName = "DirectX 12";
    std::string buildConfig = "Debug x64";
    std::string engineVersion = "DeltaEngine 0.1.0";

    void Draw();
};

DELTA_ENGINE_NS_END
