#include <iostream>
#include "Runtime/DeltaEngine.h"

#define SCREEN_WIDTH   1280
#define SCREEN_HEIGHT  720

using namespace std;


int main() {
    // initSDL();
    // while (true) {
    //     
    // }

	DeltaEngine* engine = new DeltaEngine();
    engine->Initialize();
    printf("Delta Engine Init");

    engine->StartMainLoop();


    return 0;
}