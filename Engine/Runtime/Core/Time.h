#pragma once

#include "EngineIncludes.h"

#include <chrono>
#include <wrl.h>

DELTA_ENGINE_NS_BEGIN

class Time
{
public:
    Time();
    ~Time();

    void TickTime();

    static float deltaTime;
    static float timeSinceStart;
    static UINT64 frameSinceStart;

private:
    std::chrono::high_resolution_clock clock;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastTime;
};

DELTA_ENGINE_NS_END
