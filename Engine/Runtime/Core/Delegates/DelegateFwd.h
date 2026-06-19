#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

struct FDelegateHandle;
class FDynamicMulticastDelegate;

template<typename Signature>
class TDelegate;

template<typename Ret, typename... Args>
class TDelegate<Ret(Args...)>;

template<typename Signature>
class TMulticastDelegate;

template<typename... Args>
class TMulticastDelegate<void(Args...)>;

DELTA_ENGINE_NS_END
