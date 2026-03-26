#pragma once

#include "Runtime/Logging/LogCategory.h"

#include <filesystem>

class LoggingManager
{
public:
    // Call once, very early in startup (before other subsystems)
    static void Initialize(const std::filesystem::path& logDir = "Logs");
    static void Shutdown();

    // Bolt on a custom sink after init (e.g. an in-editor console sink)
    static void AddSink(spdlog::sink_ptr sink);

    // Override every category's level at runtime (useful for -verbose flag)
    static void SetGlobalLevel(ELogLevel level);

    [[nodiscard]] static bool IsInitialized() { return s_initialized; }

private:
    static inline std::vector<spdlog::sink_ptr> s_sinks;
    static inline bool s_initialized = false;
};
