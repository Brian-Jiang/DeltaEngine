#include "Runtime/Logging/LogCategory.h"

#include <algorithm>
#include <ranges>
#include <spdlog/sinks/stdout_color_sinks.h>

std::vector<DLogCategory*>& DLogCategory::GetAllCategories()
{
    // Function-local static avoids SIOF with other static DLogCategory instances
    static std::vector<DLogCategory*> s_categories;
    return s_categories;
}

DLogCategory::DLogCategory(std::string_view name, ELogLevel defaultLevel)
    : m_name(name)
    , m_level(defaultLevel)
{
    // Fallback sink — active until LoggingManager::Initialize() replaces it.
    // This way any DLOG() calls before engine startup aren't silently dropped.
    auto fallback = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    m_logger = std::make_shared<spdlog::logger>(m_name, std::move(fallback));
    m_logger->set_level(ToSpdlogLevel(m_level));
    spdlog::register_logger(m_logger);

    GetAllCategories().push_back(this);
}

DLogCategory::~DLogCategory()
{
    auto& cats = GetAllCategories();
    std::erase(cats, this);
    spdlog::drop(m_name);
}

void DLogCategory::SetLevel(ELogLevel level)
{
    m_level = level;
    if (m_logger)
        m_logger->set_level(ToSpdlogLevel(level));
}

void DLogCategory::ReinitializeWithSinks(const std::vector<spdlog::sink_ptr>& sinks)
{
    spdlog::drop(m_name);
    m_logger = std::make_shared<spdlog::logger>(m_name, sinks.begin(), sinks.end());
    m_logger->set_level(ToSpdlogLevel(m_level));
    spdlog::register_logger(m_logger);
}
