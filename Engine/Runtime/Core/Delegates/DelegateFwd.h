#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

template<typename Signature>
class TDelegate;

template<typename Ret, typename... Args>
class TDelegate<Ret(Args...)>;

DELTA_ENGINE_NS_END
