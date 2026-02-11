#include <iostream>

#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DXUtils.h"

using namespace std;
using namespace DeltaEngine;

int main()
{
	auto engine = new EngineMain();
    engine->Initialize();
    printf("Delta Engine Init");

    engine->StartMainLoop();
    auto returnCode = engine->exitCode;

    delete engine;

    //DXUtils::ReportLiveDXGIObjects();

    return returnCode;
}