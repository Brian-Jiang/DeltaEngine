#pragma once

#include "EngineIncludes.h"

// Engine-private log channels. Not exported and not part of any umbrella
// include: only DeltaEngine translation units may include this header.

DELTA_ENGINE_NS_BEGIN

DECLARE_LOG_CATEGORY(LogCore)
DECLARE_LOG_CATEGORY(LogReflection)
DECLARE_LOG_CATEGORY(LogSerialization)
DECLARE_LOG_CATEGORY(LogAsset)
DECLARE_LOG_CATEGORY(LogRenderer)
DECLARE_LOG_CATEGORY(LogShader)
DECLARE_LOG_CATEGORY(LogRHI)
DECLARE_LOG_CATEGORY(LogPostProcess)
DECLARE_LOG_CATEGORY(LogShadow)
DECLARE_LOG_CATEGORY(LogIO)
DECLARE_LOG_CATEGORY(LogEngine)

DELTA_ENGINE_NS_END
