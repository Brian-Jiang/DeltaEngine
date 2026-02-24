#pragma once

#ifndef NOMINMAX   /* don't define min() and max(). */
#define NOMINMAX
#endif

#include "Macros.h"

DELTA_ENGINE_NS_BEGIN

template <typename T>
T* CreateDObject()
{
    return nullptr;
}

DELTA_ENGINE_NS_END
