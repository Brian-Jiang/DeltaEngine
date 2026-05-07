#include "Runtime/Logging/LogCategory.h"
#include "Runtime/Logging/LogChannels.h"
#include "Runtime/Logging/LoggingManager.h"

#include <gtest/gtest.h>
#include <spdlog/sinks/base_sink.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>

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

    DLOG(LogCore, ELogLevel::Display, "AddSink probe");
    DLOG(LogIO, ELogLevel::Display, "AddSink probe IO");

    EXPECT_GE(sink->count.load(), 2);
}

TEST_F(LoggingManagerFixture, LoggingManager_SetGlobalLevel_FiltersAllCategories)
{
    LoggingManager::Initialize(UniqueTempLogDir());
    auto sink = std::make_shared<CountingSink>();
    LoggingManager::AddSink(sink);

    LoggingManager::SetGlobalLevel(ELogLevel::Error);

    const int before = sink->count.load();
    DLOG(LogCore, ELogLevel::Log, "should be filtered");
    DLOG(LogIO, ELogLevel::Warning, "should be filtered");
    EXPECT_EQ(sink->count.load(), before);

    DLOG(LogCore, ELogLevel::Error, "should pass");
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
