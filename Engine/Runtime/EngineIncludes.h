#pragma once

#ifndef NOMINMAX   /* don't define min() and max(). */
#define NOMINMAX
#endif /* NOMINMAX */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN   // Exclude rarely-used stuff from Windows headers
#endif /* WIN32_LEAN_AND_MEAN */


#include "Macros.h"
#include "Runtime/Logging/LogCategory.h"
#include "Runtime/Logging/LoggingInternal.h"
#include "Runtime/Assert/Assert.h"

#include <typeinfo>

DELTA_ENGINE_NS_BEGIN

template <typename T>
T* CreateDObject()
{
    LoggingInternal::LogMissingCreateDObjectSpecialization(typeid(T).name());
    return nullptr;
}

DELTA_ENGINE_NS_END
