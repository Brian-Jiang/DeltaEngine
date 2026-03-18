#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class DWorld;

/// Identifies the purpose of a world instance.
enum class WorldType
{
    Editor,
};

/// Associates a DWorld with its runtime purpose.
/// EngineMain maintains a list of these to support multiple worlds
/// (e.g., editor world, game world) in the future.
struct WorldContext
{
    WorldType type  = WorldType::Editor;
    DWorld*   world = nullptr;
};

DELTA_ENGINE_NS_END
