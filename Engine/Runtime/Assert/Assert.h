#pragma once

#include <cstdlib>
#include <format>

#include "Logging/LogChannels.h" // your existing DLOG

// ─── Platform break ──────────────────────────────────────────────────────────
#if defined(_MSC_VER)
#define DELTA_DEBUG_BREAK() __debugbreak()
#else
#define DELTA_DEBUG_BREAK() __builtin_trap()
#endif

// ─── Internal handler ────────────────────────────────────────────────────────
// Logs, breaks, aborts. Never inlined so the callstack stays readable.
[[noreturn]] DELTAENGINE_API void DeltaAssertFailed(const char* expr,
                                                    const char* file,
                                                    int line,
                                                    const char* msg = nullptr);

// ─── DELTA_ASSERT (debug/dev only, fatal) ────────────────────────────────────
#ifndef DELTA_SHIPPING
#define DELTA_ASSERT(expr)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
            DeltaAssertFailed(#expr, __FILE__, __LINE__);                                                              \
    } while (false)

#define DELTA_ASSERT_MSG(expr, fmt, ...)                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
            DeltaAssertFailed(#expr, __FILE__, __LINE__, std::format(fmt, ##__VA_ARGS__).c_str());                     \
    } while (false)
#else
#define DELTA_ASSERT(expr)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        (void)(expr);                                                                                                  \
    } while (false)
#define DELTA_ASSERT_MSG(expr, fmt, ...)                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        (void)(expr);                                                                                                  \
    } while (false)
#endif

// ─── DELTA_VERIFY (all builds, fatal) ────────────────────────────────────────
#define DELTA_VERIFY(expr)                                                                                             \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
            DeltaAssertFailed(#expr, __FILE__, __LINE__);                                                              \
    } while (false)

#define DELTA_VERIFY_MSG(expr, fmt, ...)                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
            DeltaAssertFailed(#expr, __FILE__, __LINE__, std::format(fmt, ##__VA_ARGS__).c_str());                     \
    } while (false)

// ─── DELTA_ENSURE (all builds, non-fatal) ────────────────────────────────────
// Returns the expression result so you can write: if (!DELTA_ENSURE(ptr)) return;
namespace DeltaInternal
{
DELTAENGINE_API bool EnsureFailed(const char *expr, const char *file, int line, const char *msg = nullptr);
}

#define DELTA_ENSURE(expr) ((expr) || DeltaInternal::EnsureFailed(#expr, __FILE__, __LINE__))

#define DELTA_ENSURE_MSG(expr, fmt, ...)                                                                               \
    ((expr) || DeltaInternal::EnsureFailed(#expr, __FILE__, __LINE__, std::format(fmt, ##__VA_ARGS__).c_str()))

// ─── DELTA_CHECK (debug/dev only, non-fatal debugger break) ──────────────────
#ifndef DELTA_SHIPPING
#define DELTA_CHECK(expr)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            DLOG(LogCore, ELogLevel::Error, "CHECK failed: {} ({}:{})", #expr, __FILE__, __LINE__);                    \
            DELTA_DEBUG_BREAK();                                                                                       \
        }                                                                                                              \
    } while (false)
#define DELTA_CHECK_MSG(expr, fmt, ...)                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(expr))                                                                                                   \
        {                                                                                                              \
            DLOG(LogCore, ELogLevel::Error, "CHECK failed: {} — {} ({}:{})", #expr, std::format(fmt, ##__VA_ARGS__),   \
                 __FILE__, __LINE__);                                                                                  \
            DELTA_DEBUG_BREAK();                                                                                       \
        }                                                                                                              \
    } while (false)
#else
#define DELTA_CHECK(expr)                                                                                              \
    do                                                                                                                 \
    {                                                                                                                  \
        (void)(expr);                                                                                                  \
    } while (false)
#define DELTA_CHECK_MSG(expr, fmt, ...)                                                                                \
    do                                                                                                                 \
    {                                                                                                                  \
        (void)(expr);                                                                                                  \
    } while (false)
#endif

// ─── Unconditional ───────────────────────────────────────────────────────────
#define DELTA_UNREACHABLE() DELTA_ASSERT_MSG(false, "Unreachable code reached")
#define DELTA_NO_ENTRY() DELTA_ASSERT_MSG(false, "No-entry code path executed")
#define DELTA_NOT_IMPLEMENTED() DELTA_ASSERT_MSG(false, "Not implemented")
