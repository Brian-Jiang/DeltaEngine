#include "EngineIncludes.h"

#include "EditorMain.h"

using namespace DeltaEngine;

extern "C"
{
    __declspec(dllexport) extern const UINT D3D12SDKVersion = 619;
}

extern "C"
{
    __declspec(dllexport) extern const char *D3D12SDKPath = ".\\D3D12\\";
}

int main()
{
    EditorMain editor;
    return editor.Run();
}
