#include "Runtime/Assert/Assert.h"

#include <cstdio>

[[noreturn]] void DeltaAssertFailed(const char *expr, const char *file, int line, const char *msg)
{
    if (msg)
        DLOG(LogCore, ELogLevel::Fatal, "ASSERT failed: {} — {}\n  {}:{}", expr, msg, file, line);
    else
        DLOG(LogCore, ELogLevel::Fatal, "ASSERT failed: {}\n  {}:{}", expr, file, line);

    DELTA_DEBUG_BREAK();
    std::abort();
}

namespace DeltaInternal
{
bool EnsureFailed(const char *expr, const char *file, int line, const char *msg)
{
    // One-time firing per call site — thread-safe with a static local
    static thread_local bool fired = false;
    if (!fired)
    {
        fired = true;
        if (msg)
            DLOG(LogCore, ELogLevel::Warning, "ENSURE failed: {} — {}\n  {}:{}", expr, msg, file, line);
        else
            DLOG(LogCore, ELogLevel::Warning, "ENSURE failed: {}\n  {}:{}", expr, file, line);
    }
    return false; // so the || short-circuit works in the macro
}
} // namespace DeltaInternal
