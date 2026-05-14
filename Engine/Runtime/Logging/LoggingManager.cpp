#include "Runtime/Logging/LoggingManager.h"

#include "Runtime/Assert/Assert.h"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <chrono>
#include <format>
#include <system_error>

std::vector<spdlog::sink_ptr> LoggingManager::s_sinks;
std::filesystem::path LoggingManager::s_logFilePath;
bool LoggingManager::s_initialized = false;

bool LoggingManager::IsInitialized()
{
    return s_initialized;
}

std::filesystem::path LoggingManager::GetCurrentLogFilePath()
{
    return s_logFilePath;
}

// [14:23:01.234] [LogRenderer] [warning] Your message here
static constexpr const char* k_pattern = "[%T.%e] [%n] %^[%l]%$ %v";
static constexpr std::size_t k_maxBytes = 5 * 1024 * 1024; // 5 MB per file
static constexpr std::size_t k_maxFiles = 3; // keep 3 rotations

void LoggingManager::Initialize(const std::filesystem::path& logDir)
{
    if (s_initialized)
    {
        DLOG(LogCore, ELogLevel::Warning,
            "LoggingManager::Initialize called twice (logDir='{}'), ignoring",
            logDir.string());
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(logDir, ec);
    if (ec)
    {
        DLOG(LogCore, ELogLevel::Error,
            "LoggingManager::Initialize failed to create log directory '{}': {}",
            logDir.string(), ec.message());
        return;
    }

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern(k_pattern);

    const auto now = std::chrono::system_clock::now();
    const auto logPath = logDir / std::format("DeltaEngine_{:%Y%m%d_%H%M%S}.log", now);
    s_logFilePath = logPath;
    auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        logPath.string(), k_maxBytes, k_maxFiles);
    fileSink->set_pattern(k_pattern);

    s_sinks = { consoleSink, fileSink };

    for (DLogCategory* cat : DLogCategory::GetAllCategories())
        cat->ReinitializeWithSinks(s_sinks);

    s_initialized = true;

    DLOG(LogCore, ELogLevel::Display,
        "LoggingManager initialized: logDir='{}', file='{}', categories={}",
        logDir.string(), logPath.filename().string(),
        DLogCategory::GetAllCategories().size());
}

void LoggingManager::Shutdown()
{
    if (!s_initialized)
        return;

    DLOG(LogCore, ELogLevel::Display, "LoggingManager shutting down");
    spdlog::shutdown();
    s_sinks.clear();
    s_logFilePath.clear();
    s_initialized = false;
}

void LoggingManager::AddSink(spdlog::sink_ptr sink)
{
    if (!DELTA_ENSURE_MSG(sink != nullptr, "LoggingManager::AddSink called with null sink"))
        return;
    DELTA_CHECK_MSG(s_initialized, "LoggingManager::AddSink called before Initialize");

    sink->set_pattern(k_pattern);
    s_sinks.push_back(sink);
    for (DLogCategory* cat : DLogCategory::GetAllCategories())
        cat->GetLogger()->sinks().push_back(sink);
}

void LoggingManager::SetGlobalLevel(ELogLevel level)
{
    for (DLogCategory* cat : DLogCategory::GetAllCategories())
        cat->SetLevel(level);
}
