#pragma once

#include "Runtime/Logging/LogCategory.h"

#include <filesystem>

class LoggingManager
{
public:
    DELTAENGINE_API static void Initialize(const std::filesystem::path& logDir = "Logs");
    DELTAENGINE_API static void Shutdown();
    DELTAENGINE_API static void AddSink(spdlog::sink_ptr sink);
    DELTAENGINE_API static void SetGlobalLevel(ELogLevel level);

    [[nodiscard]] DELTAENGINE_API static bool IsInitialized();

private:
    static std::vector<spdlog::sink_ptr> s_sinks;
    static bool s_initialized;
};
