#pragma once

#define DELTA_ENGINE_NS_BEGIN namespace DeltaEngine {
#define DELTA_ENGINE_NS_END }


#ifdef _WIN32
    #ifdef DELTAENGINE_EXPORTS
        #define DELTAENGINE_API __declspec(dllexport)
    #else
        #define DELTAENGINE_API __declspec(dllimport)
    #endif
#else
    #define DELTAENGINE_API __attribute__((visibility("default")))
#endif