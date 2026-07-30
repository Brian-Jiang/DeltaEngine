#pragma once

#include "Runtime/Macros.h"

#include <memory>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>

DELTA_ENGINE_NS_BEGIN

// ---------------------------------------------------------------------------
// Log levels — mirrors UE5's hierarchy
// ---------------------------------------------------------------------------
enum class ELogLevel : uint8_t
{
    VeryVerbose = 0, // spdlog trace
    Verbose = 1, // spdlog debug
    Log = 2, // spdlog info
    Display = 3, // spdlog info  (visible in non-development builds)
    Warning = 4, // spdlog warn
    Error = 5, // spdlog err
    Fatal = 6, // spdlog critical + std::abort()
};

constexpr spdlog::level::level_enum ToSpdlogLevel(ELogLevel level) noexcept
{
    using L = spdlog::level::level_enum;
    switch (level)
    {
    case ELogLevel::VeryVerbose:
        return L::trace;
    case ELogLevel::Verbose:
        return L::debug;
    case ELogLevel::Log:
        return L::info;
    case ELogLevel::Display:
        return L::info;
    case ELogLevel::Warning:
        return L::warn;
    case ELogLevel::Error:
        return L::err;
    case ELogLevel::Fatal:
        return L::critical;
    default:
        return L::info;
    }
}

// ---------------------------------------------------------------------------
// DLogCategory
// ---------------------------------------------------------------------------
class DELTAENGINE_API DLogCategory
{
public:
    DLogCategory(std::string_view name, ELogLevel defaultLevel);
    ~DLogCategory();

    DLogCategory(const DLogCategory&) = delete;
    DLogCategory& operator=(const DLogCategory&) = delete;

    [[nodiscard]] spdlog::logger* GetLogger() const { return m_logger.get(); }
    [[nodiscard]] ELogLevel GetLevel() const { return m_level; }
    [[nodiscard]] const std::string& GetName() const { return m_name; }

    void SetLevel(ELogLevel level);
    void ReinitializeWithSinks(const std::vector<spdlog::sink_ptr>& sinks);

    static std::vector<DLogCategory*>& GetAllCategories();

    /**
     * Looks a category up by name; returns nullptr if no such category is registered.
     * Categories are module-private and never exported, so this is how one module
     * observes or configures another module's categories.
     */
    static DLogCategory* FindByName(std::string_view name);

private:
    std::shared_ptr<spdlog::logger> m_logger;
    std::string m_name;
    ELogLevel m_level;
};

DELTA_ENGINE_NS_END

// ---------------------------------------------------------------------------
// Declaration / Definition macros  (identical pattern to UE5)
//
// Log categories are module-private: they are never exported across a DLL
// boundary, so a category may only be used inside the module that defines it.
// Use DLogCategory::FindByName to observe another module's category.
// ---------------------------------------------------------------------------

// In a module-private .h — forward-declares the category for other TUs in the same module
#define DECLARE_LOG_CATEGORY(CategoryName) \
    extern ::DeltaEngine::DLogCategory CategoryName;

// In a .cpp — defines a module-wide category
#define DEFINE_LOG_CATEGORY(CategoryName) \
    ::DeltaEngine::DLogCategory CategoryName { #CategoryName, ::DeltaEngine::ELogLevel::Log };

// In a .cpp — file-local category, no header needed
#define DEFINE_LOG_CATEGORY_STATIC(CategoryName) \
    static ::DeltaEngine::DLogCategory CategoryName { #CategoryName, ::DeltaEngine::ELogLevel::Log }

// ---------------------------------------------------------------------------
// DLOG — primary logging macro
// Usage: DLOG(LogRenderer, Warning, "Mesh {} failed to load", meshName);
// ---------------------------------------------------------------------------
#define DLOG(Category, Level, ...)                                                        \
    do {                                                                                  \
        constexpr ::spdlog::level::level_enum _lvl = ::DeltaEngine::ToSpdlogLevel(Level); \
        auto* _log = (Category).GetLogger();                                              \
        if (_log && _log->should_log(_lvl)) {                                             \
            _log->log(spdlog::source_loc { __FILE__, __LINE__, __func__ },                \
                _lvl, __VA_ARGS__);                                                       \
        }                                                                                 \
        if constexpr (Level == ::DeltaEngine::ELogLevel::Fatal)                           \
            std::abort();                                                                 \
    } while (false)

// Conditional variant — logs only when Cond is true
#define DLOG_IF(Category, Level, Cond, ...)     \
    do {                                        \
        if (Cond) {                             \
            DLOG(Category, Level, __VA_ARGS__); \
        }                                       \
    } while (false)
