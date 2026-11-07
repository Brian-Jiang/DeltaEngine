#pragma once

#include "EngineIncludes.h"

#include <filesystem>
#include <optional>

DELTA_ENGINE_NS_BEGIN

class LoggingManager
{
public:
    DELTAENGINE_API static void Initialize(const std::filesystem::path& logDir = "Logs");
    DELTAENGINE_API static void Shutdown();
    DELTAENGINE_API static void AddSink(spdlog::sink_ptr sink);
    DELTAENGINE_API static void SetGlobalLevel(ELogLevel level);
    DELTAENGINE_API static void FlushAll();

    [[nodiscard]] DELTAENGINE_API static bool IsInitialized();

    /** Absolute path of the active log file; empty if not initialized. */
    [[nodiscard]] DELTAENGINE_API static std::filesystem::path GetCurrentLogFilePath();

    /** Gives a category created after Initialize (e.g. in a late-loaded DLL) the shared sinks and global level. */
    static void AttachLateCategory(DLogCategory& category);

private:
    static std::optional<ELogLevel> s_globalLevel;
    static std::vector<spdlog::sink_ptr> s_sinks;
    static std::filesystem::path s_logFilePath;
    static bool s_initialized;
};

DELTA_ENGINE_NS_END
