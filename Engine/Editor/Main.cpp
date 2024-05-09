#include <iostream>
#include "Runtime/DeltaEngine.h"

using namespace std;

int main() {
	DeltaEngine* engine = new DeltaEngine();
    engine->Initialize();
    printf("Delta Engine Init");

    engine->StartMainLoop();


    return engine->exitCode;
}