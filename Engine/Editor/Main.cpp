#include <iostream>

#include "Runtime/EngineLaunch.h"

using namespace std;
using namespace DeltaEngine;

int main() {
	auto engine = new EngineLaunch();
    engine->Initialize();
    printf("Delta Engine Init");

    engine->StartMainLoop();

    return engine->exitCode;
}