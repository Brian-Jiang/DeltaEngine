#pragma once

// DLL export/import macros
#ifdef _WIN32
    #ifdef DELTAEDITOR_EXPORTS
        #define DELTAEDITOR_API __declspec(dllexport)
    #else
        #define DELTAEDITOR_API __declspec(dllimport)
    #endif
#else
    #define DELTAEDITOR_API __attribute__((visibility("default")))
#endif