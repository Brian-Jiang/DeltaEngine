#include "Editor/Mcp/McpCoreFixture.h"

#include "Runtime/Logging/LogChannels.h"
#include "Runtime/Logging/LoggingManager.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

using json = nlohmann::json;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class McpLogSystemTests : public McpCoreFixture
{
protected:
    static std::string UniqueMarker()
    {
        static std::atomic<uint64_t> counter{ 0 };
        return "MCP_LOG_MARKER_" + std::to_string(counter.fetch_add(1));
    }
};

TEST_F(McpLogSystemTests, FileLocation_ReturnsExistingPath)
{
    DLOG(LogCore, ELogLevel::Display, "force-flush-marker");

    auto res = Dispatch("log", "file_location");
    ASSERT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["path"].is_string());
    EXPECT_FALSE(res["path"].get<std::string>().empty());
    EXPECT_TRUE(res["exists"].get<bool>());
    EXPECT_TRUE(std::filesystem::exists(res["path"].get<std::string>()));
}

TEST_F(McpLogSystemTests, Read_ReturnsRecentEntries)
{
    const std::string marker = UniqueMarker();
    DLOG(LogCore, ELogLevel::Warning, "{}", marker);

    json params;
    params["text_search"] = marker;
    params["min_severity"] = "VeryVerbose";
    auto res = Dispatch("log", "read", params);
    ASSERT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["entries"].is_array());
    ASSERT_GE(res["entries"].size(), 1u);

    bool foundWarning = false;
    for (const auto& e : res["entries"])
    {
        if (e["message"].get<std::string>().find(marker) != std::string::npos)
        {
            EXPECT_EQ(e["level"].get<std::string>(), "Warning");
            foundWarning = true;
        }
    }
    EXPECT_TRUE(foundWarning);
}

TEST_F(McpLogSystemTests, Read_FiltersBySeverity)
{
    const std::string marker = UniqueMarker();
    DLOG(LogCore, ELogLevel::Log, "{} info-line", marker);
    DLOG(LogCore, ELogLevel::Error, "{} err-line", marker);

    json params;
    params["text_search"]  = marker;
    params["min_severity"] = "Error";
    auto res = Dispatch("log", "read", params);
    ASSERT_TRUE(res["ok"].get<bool>());

    int matchedError = 0;
    int matchedInfo  = 0;
    for (const auto& e : res["entries"])
    {
        const std::string msg = e["message"].get<std::string>();
        if (msg.find(marker) == std::string::npos)
            continue;
        if (e["level"].get<std::string>() == "Error")
            ++matchedError;
        if (e["level"].get<std::string>() == "Log")
            ++matchedInfo;
    }
    EXPECT_GE(matchedError, 1);
    EXPECT_EQ(matchedInfo, 0);
}

TEST_F(McpLogSystemTests, Read_RespectsCountClamp)
{
    const std::string marker = UniqueMarker();
    for (int i = 0; i < 5; ++i)
        DLOG(LogCore, ELogLevel::Warning, "{} idx={}", marker, i);

    json params;
    params["text_search"]  = marker;
    params["min_severity"] = "VeryVerbose";
    params["count"]        = 2;
    auto res = Dispatch("log", "read", params);
    ASSERT_TRUE(res["ok"].get<bool>());
    ASSERT_TRUE(res["entries"].is_array());
    EXPECT_EQ(res["entries"].size(), 2u);
}

TEST_F(McpLogSystemTests, Read_FiltersByCategory)
{
    const std::string marker = UniqueMarker();
    DLOG(LogCore, ELogLevel::Warning, "{}", marker);

    json params;
    params["text_search"]  = marker;
    params["category"]     = "Core";
    params["min_severity"] = "VeryVerbose";
    auto res = Dispatch("log", "read", params);
    ASSERT_TRUE(res["ok"].get<bool>());
    ASSERT_GE(res["entries"].size(), 1u);
    for (const auto& e : res["entries"])
    {
        const std::string cat = e["category"].get<std::string>();
        EXPECT_NE(cat.find("Core"), std::string::npos);
    }
}

TEST_F(McpLogSystemTests, Read_InvalidSeverity_ReturnsError)
{
    json params;
    params["min_severity"] = "not-a-level";
    auto res = Dispatch("log", "read", params);
    EXPECT_FALSE(res["ok"].get<bool>());
}
