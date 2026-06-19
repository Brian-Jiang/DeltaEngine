#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/Delegates/DynamicDelegate.h"
#include "Runtime/Macros.h"

#include "DynamicDelegateTestTypes.generated.h"

DELTA_ENGINE_NS_BEGIN

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIntEvent, int, value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTwoArgEvent, int, a, float, b);

DELTA_ENGINE_NS_END
