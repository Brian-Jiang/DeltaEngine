#include <iostream>

#include "Runtime/EngineMain.h"

using namespace std;
using namespace DeltaEngine;

int main() {
	auto engine = new EngineMain();
    engine->Initialize();
    printf("Delta Engine Init");

    engine->StartMainLoop();

    return engine->exitCode;
}