#pragma once

#ifndef NOMINMAX   /* don't define min() and max(). */
#define NOMINMAX
#endif /* NOMINMAX */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN   // Exclude rarely-used stuff from Windows headers
#endif /* WIN32_LEAN_AND_MEAN */


#include "Macros.h"
#include "Runtime/Logging/LogChannels.h"
#include "Runtime/Assert/Assert.h"

DELTA_ENGINE_NS_BEGIN

template <typename T>
T* CreateDObject()
{
    DLOG(LogCore, ELogLevel::Error, "CreateDObject called without a specialization for type %s", typeid(T).name());
    return nullptr;
}

DELTA_ENGINE_NS_END
