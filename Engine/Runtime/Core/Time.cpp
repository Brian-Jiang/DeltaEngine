#include "Core/Time.h"

using namespace DeltaEngine;

float Time::deltaTime = 0.0f;
float Time::timeSinceStart = 0.0f;

Time::Time() {
    lastTime = clock.now();
    deltaTime = 0.0f;
    timeSinceStart = 0.0f;
}

Time::~Time() {

}

void Time::TickTime() {
    auto currentTime = clock.now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime);
    lastTime = currentTime;
    deltaTime = duration.count() / 1000000.0f;
    timeSinceStart += deltaTime;
}
