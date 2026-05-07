#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/UUID.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Logging/LogCategory.h"
#include "Runtime/Logging/LogChannels.h"

#include <gtest/gtest.h>
#include <spdlog/sinks/base_sink.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

using namespace DeltaEngine;

namespace
{
class CapturingSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
    std::atomic<int> warningCount { 0 };
    std::atomic<int> errorCount { 0 };
    std::string lastMessage;

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        if (msg.level == spdlog::level::warn)
            warningCount.fetch_add(1, std::memory_order_relaxed);
        else if (msg.level == spdlog::level::err)
            errorCount.fetch_add(1, std::memory_order_relaxed);
        lastMessage.assign(msg.payload.data(), msg.payload.size());
    }
    void flush_() override {}
};
}

class EngineMainFixture : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_sink = std::make_shared<CapturingSink>();
        LogEngine.GetLogger()->sinks().push_back(m_sink);
        LogEngine.SetLevel(ELogLevel::Verbose);
    }

    void TearDown() override
    {
        auto& sinks = LogEngine.GetLogger()->sinks();
        std::erase_if(sinks, [this](const auto& s) { return s.get() == m_sink.get(); });
    }

    std::shared_ptr<CapturingSink> m_sink;
};

TEST_F(EngineMainFixture, EngineMain_GetWorld_ReturnsNull_WhenNoContextRegistered)
{
    EngineMain engine;

    EXPECT_EQ(engine.GetWorld(), nullptr);
}

TEST_F(EngineMainFixture, EngineMain_CreateWorld_RegistersEditorWorldContext)
{
    EngineMain engine;

    engine.CreateWorld();

    EXPECT_NE(engine.GetWorld(), nullptr);
}

TEST_F(EngineMainFixture, EngineMain_CreateWorld_CalledTwice_LogsWarningAndKeepsFirstWorld)
{
    EngineMain engine;
    engine.CreateWorld();
    DWorld* first = engine.GetWorld();
    const int warnsBefore = m_sink->warningCount.load();

    engine.CreateWorld();

    EXPECT_EQ(engine.GetWorld(), first);
    EXPECT_GT(m_sink->warningCount.load(), warnsBefore);
}

TEST_F(EngineMainFixture, EngineMain_GetCamera_ReturnsNull_WhenNoCameraGameObject)
{
    EngineMain engine;
    engine.CreateWorld();

    EXPECT_EQ(engine.GetCamera(), nullptr);
}

TEST_F(EngineMainFixture, EngineMain_OnWindowResized_ZeroHeight_LogsWarningAndDoesNotCrash)
{
    EngineMain engine;
    const int warnsBefore = m_sink->warningCount.load();

    engine.OnWindowResized(800, 0);

    EXPECT_GT(m_sink->warningCount.load(), warnsBefore);
}

TEST_F(EngineMainFixture, EngineMain_LoadScene_NoWorld_LogsErrorAndReturns)
{
    EngineMain engine;
    const int errorsBefore = m_sink->errorCount.load();

    engine.LoadScene(AssetId {});

    EXPECT_GT(m_sink->errorCount.load(), errorsBefore);
}

TEST_F(EngineMainFixture, EngineMain_Cleanup_OnFreshlyConstructed_DoesNothing)
{
    EngineMain engine;

    engine.Cleanup();

    EXPECT_EQ(engine.GetWorld(), nullptr);
}
