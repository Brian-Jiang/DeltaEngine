#pragma once

#include "EngineIncludes.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT
#include <stdexcept>

DELTA_ENGINE_NS_BEGIN

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
static inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        char errorMsg[512];
        sprintf_s(errorMsg, "DirectX Error: File: %s, Line: %d, Function: %s, HRESULT: 0x%08X\n",
                  __FILE__, __LINE__, __FUNCTION__, static_cast<unsigned int>(hr));
        OutputDebugStringA(errorMsg);
        DLOG(LogRHI, ELogLevel::Error, "DirectX call failed at {}:{} in {} hr=0x{:08X}",
            __FILE__, __LINE__, __FUNCTION__, static_cast<unsigned int>(hr));
        throw std::runtime_error(errorMsg);
    }
}

#define _KB(x) (x * 1024)
#define _MB(x) (x * 1024 * 1024)

#define _64KB _KB(64)
#define _1MB _MB(1)
#define _2MB _MB(2)
#define _4MB _MB(4)
#define _8MB _MB(8)
#define _16MB _MB(16)
#define _32MB _MB(32)
#define _64MB _MB(64)
#define _128MB _MB(128)
#define _256MB _MB(256)


DELTA_ENGINE_NS_END
