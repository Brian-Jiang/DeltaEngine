#pragma once

#include "Runtime/Macros.h"

#include <cstddef>

DELTA_ENGINE_NS_BEGIN

namespace LoggingInternal
{

/**
 * Relays for log sites that live in headers (inline or template code) and would
 * otherwise be compiled into other modules. Log categories are module-private and
 * never exported, so header code must log through an exported function instead.
 */

DELTAENGINE_API void LogMissingCreateDObjectSpecialization(const char* typeName);

/** Pass a null key for nested arrays, which have no key of their own. */
DELTAENGINE_API void LogArrayResizeFailure(const char* key, std::size_t requested, const char* what);

} // namespace LoggingInternal

DELTA_ENGINE_NS_END
