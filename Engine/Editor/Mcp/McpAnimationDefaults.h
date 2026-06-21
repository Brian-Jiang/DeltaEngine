#pragma once

DELTA_ENGINE_NS_BEGIN

/// Default tween duration (seconds) for MCP commands that support editor animation
/// when the caller omits `duration_seconds`. A caller-supplied value <= 0 means instant.
inline constexpr float kDefaultAnimationDurationSeconds = 1.0f;

DELTA_ENGINE_NS_END
