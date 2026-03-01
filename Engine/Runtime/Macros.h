#pragma once


// Common macros for the Delta Engine
#define DELTA_ENGINE_NS_BEGIN namespace DeltaEngine {
#define DELTA_ENGINE_NS_END }


// DLL export/import macros
#ifdef _WIN32
    #ifdef DELTAENGINE_EXPORTS
        #define DELTAENGINE_API __declspec(dllexport)
    #else
        #define DELTAENGINE_API __declspec(dllimport)
    #endif
#else
    #define DELTAENGINE_API __attribute__((visibility("default")))
#endif


// Reflection macros
#define DCLASS(...)
#define DSTRUCT(...)
#define DFUNCTION(...)
#define DPROPERTY(...)
#define DGENERATED_BODY(ClassName) \
    friend class DeltaEngine::Reflection::Private::ReflectionRegister_##ClassName;
#define DGENERATED_BODY_STRUCT(StructName) \
    friend class DeltaEngine::Reflection::Private::ReflectionRegister_##StructName;
