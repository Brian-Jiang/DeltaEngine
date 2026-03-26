#include "Runtime/Logging/LoggingManager.h"

#include <chrono>
#include <format>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// [14:23:01.234] [LogRenderer] [warning] Your message here
static constexpr const char* k_pattern = "[%T.%e] [%n] %^[%l]%$ %v";
static constexpr std::size_t k_maxBytes = 5 * 1024 * 1024; // 5 MB per file
static constexpr std::size_t k_maxFiles = 3; // keep 3 rotations

void LoggingManager::Initialize(const std::filesystem::path& logDir)
{
    if (s_initialized)
        return;

    std::filesystem::create_directories(logDir);

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern(k_pattern);

    // Timestamped log file so runs don't clobber each other
    const auto now = std::chrono::system_clock::now();
    const auto logPath = logDir / std::format("DeltaEngine_{:%Y%m%d_%H%M%S}.log", now);
    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logPath.string(), k_maxBytes, k_maxFiles);
    fileSink->set_pattern(k_pattern);

    s_sinks = { consoleSink, fileSink };

    // Upgrade every category that was constructed before this call
    for (DLogCategory* cat : DLogCategory::GetAllCategories())
        cat->ReinitializeWithSinks(s_sinks);

    s_initialized = true;
}

void LoggingManager::Shutdown()
{
    spdlog::shutdown();
    s_sinks.clear();
    s_initialized = false;
}

void LoggingManager::AddSink(spdlog::sink_ptr sink)
{
    sink->set_pattern(k_pattern); // keep formatting consistent
    s_sinks.push_back(sink);
    for (DLogCategory* cat : DLogCategory::GetAllCategories())
        cat->GetLogger()->sinks().push_back(sink);
}

void LoggingManager::SetGlobalLevel(ELogLevel level)
{
    for (DLogCategory* cat : DLogCategory::GetAllCategories())
        cat->SetLevel(level);
}
