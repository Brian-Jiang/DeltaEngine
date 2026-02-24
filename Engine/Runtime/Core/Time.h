#pragma once

#include "EngineIncludes.h"

#include <chrono>
#include <wrl.h>

DELTA_ENGINE_NS_BEGIN

class Time
{
public:
    DELTAENGINE_API Time();
    DELTAENGINE_API ~Time();

    DELTAENGINE_API void TickTime();

    static DELTAENGINE_API float deltaTime;
    static DELTAENGINE_API float timeSinceStart;
    static DELTAENGINE_API UINT64 frameSinceStart;

private:
    std::chrono::high_resolution_clock clock;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
};

DELTA_ENGINE_NS_END
