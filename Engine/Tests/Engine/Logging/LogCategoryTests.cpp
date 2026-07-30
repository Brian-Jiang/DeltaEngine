#include "Runtime/Logging/LogChannels.h"

#include <gtest/gtest.h>
#include <spdlog/sinks/base_sink.h>

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

using namespace DeltaEngine;

namespace
{
class CountingSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
    std::atomic<int> count { 0 };
    std::string lastMessage;

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        count.fetch_add(1, std::memory_order_relaxed);
        lastMessage.assign(msg.payload.data(), msg.payload.size());
    }
    void flush_() override {}
};
}

TEST(DLogCategory, DLogCategory_Constructor_RegistersInGlobalList)
{
    const std::size_t before = DLogCategory::GetAllCategories().size();

    {
        DLogCategory cat("DLogCategoryTests_TempA", ELogLevel::Log);
        EXPECT_EQ(DLogCategory::GetAllCategories().size(), before + 1);
    }

    EXPECT_EQ(DLogCategory::GetAllCategories().size(), before);
}

TEST(DLogCategory, DLogCategory_SetLevel_FiltersBelowLevel)
{
    DLogCategory cat("DLogCategoryTests_LevelFilter", ELogLevel::Log);
    auto sink = std::make_shared<CountingSink>();
    cat.ReinitializeWithSinks({ sink });

    cat.SetLevel(ELogLevel::Warning);

    DLOG(cat, ELogLevel::Log, "below threshold");
    DLOG(cat, ELogLevel::Warning, "at threshold");
    DLOG(cat, ELogLevel::Error, "above threshold");

    EXPECT_EQ(sink->count.load(), 2);
}

TEST(DLogCategory, DLogCategory_ReinitializeWithSinks_RoutesToNewSinks)
{
    DLogCategory cat("DLogCategoryTests_Reinit", ELogLevel::Log);
    auto first = std::make_shared<CountingSink>();
    cat.ReinitializeWithSinks({ first });

    DLOG(cat, ELogLevel::Log, "first");
    EXPECT_EQ(first->count.load(), 1);

    auto second = std::make_shared<CountingSink>();
    cat.ReinitializeWithSinks({ second });

    DLOG(cat, ELogLevel::Log, "second");
    EXPECT_EQ(first->count.load(), 1);
    EXPECT_EQ(second->count.load(), 1);
}

TEST(DLogCategory, DLogCategory_DestructorRemovesFromGlobalList)
{
    auto* cat = new DLogCategory("DLogCategoryTests_Lifetime", ELogLevel::Log);
    const auto& cats = DLogCategory::GetAllCategories();
    EXPECT_NE(std::find(cats.begin(), cats.end(), cat), cats.end());

    delete cat;

    EXPECT_EQ(std::find(cats.begin(), cats.end(), cat), cats.end());
}
