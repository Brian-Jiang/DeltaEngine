#include "Runtime/Logging/LoggingManager.h"

#include <gtest/gtest.h>
#include <spdlog/sinks/base_sink.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>

using namespace DeltaEngine;


// Test-local categories: log categories are module-private, so a test executable
// defines its own rather than logging into an engine channel.
DEFINE_LOG_CATEGORY_STATIC(LogLoggingManagerProbe);
DEFINE_LOG_CATEGORY_STATIC(LogLoggingManagerProbeAlt);

namespace
{
class CountingSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
    std::atomic<int> count { 0 };

protected:
    void sink_it_(const spdlog::details::log_msg&) override
    {
        count.fetch_add(1, std::memory_order_relaxed);
    }
    void flush_() override {}
};

std::filesystem::path UniqueTempLogDir()
{
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return std::filesystem::temp_directory_path() / ("DeltaLoggingManagerTests_" + std::to_string(stamp));
}
}

class LoggingManagerFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        LoggingManager::Shutdown();
    }
    void TearDown() override
    {
        LoggingManager::Shutdown();
    }
};

TEST_F(LoggingManagerFixture, LoggingManager_Initialize_CreatesLogDirectory)
{
    const auto dir = UniqueTempLogDir();
    ASSERT_FALSE(std::filesystem::exists(dir));

    LoggingManager::Initialize(dir);

    EXPECT_TRUE(LoggingManager::IsInitialized());
    EXPECT_TRUE(std::filesystem::exists(dir));

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST_F(LoggingManagerFixture, LoggingManager_DoubleInitialize_IsNoOp)
{
    const auto dir = UniqueTempLogDir();
    LoggingManager::Initialize(dir);

    LoggingManager::Initialize(UniqueTempLogDir());

    EXPECT_TRUE(LoggingManager::IsInitialized());

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST_F(LoggingManagerFixture, LoggingManager_AddSink_RoutesToAllCategories)
{
    LoggingManager::Initialize(UniqueTempLogDir());
    auto sink = std::make_shared<CountingSink>();

    LoggingManager::AddSink(sink);

    DLOG(LogLoggingManagerProbe, ELogLevel::Display, "AddSink probe");
    DLOG(LogLoggingManagerProbeAlt, ELogLevel::Display, "AddSink probe alt");

    EXPECT_GE(sink->count.load(), 2);
}

TEST_F(LoggingManagerFixture, LoggingManager_SetGlobalLevel_FiltersAllCategories)
{
    LoggingManager::Initialize(UniqueTempLogDir());
    auto sink = std::make_shared<CountingSink>();
    LoggingManager::AddSink(sink);

    LoggingManager::SetGlobalLevel(ELogLevel::Error);

    const int before = sink->count.load();
    DLOG(LogLoggingManagerProbe, ELogLevel::Log, "should be filtered");
    DLOG(LogLoggingManagerProbeAlt, ELogLevel::Warning, "should be filtered");
    EXPECT_EQ(sink->count.load(), before);

    DLOG(LogLoggingManagerProbe, ELogLevel::Error, "should pass");
    EXPECT_EQ(sink->count.load(), before + 1);

    LoggingManager::SetGlobalLevel(ELogLevel::Log);
}

TEST_F(LoggingManagerFixture, LoggingManager_Shutdown_ResetsInitializedFlag)
{
    LoggingManager::Initialize(UniqueTempLogDir());
    ASSERT_TRUE(LoggingManager::IsInitialized());

    LoggingManager::Shutdown();

    EXPECT_FALSE(LoggingManager::IsInitialized());
}
