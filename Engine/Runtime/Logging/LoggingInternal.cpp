#include "Runtime/Logging/LoggingInternal.h"

#include "Runtime/Logging/LogChannels.h"

DELTA_ENGINE_NS_BEGIN

namespace LoggingInternal
{

void LogMissingCreateDObjectSpecialization(const char* typeName)
{
    DLOG(LogCore, ELogLevel::Error, "CreateDObject called without a specialization for type {}", typeName);
}

void LogArrayResizeFailure(const char* key, std::size_t requested, const char* what)
{
    if (key)
        DLOG(LogSerialization, ELogLevel::Error,
             "Failed to resize serialized array '{}': requested {} elements but allocation failed ({})",
             key, requested, what);
    else
        DLOG(LogSerialization, ELogLevel::Error,
             "Failed to resize nested serialized array: requested {} elements but allocation failed ({})",
             requested, what);
}

} // namespace LoggingInternal

DELTA_ENGINE_NS_END
