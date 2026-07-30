#include "Runtime/Logging/LogCategory.h"

#include "Runtime/Assert/Assert.h"
#include "Runtime/Logging/LogChannels.h"

#include <spdlog/sinks/stdout_color_sinks.h>

#include <algorithm>
#include <ranges>

using namespace DeltaEngine;

DELTA_ENGINE_NS_BEGIN

std::vector<DLogCategory*>& DLogCategory::GetAllCategories()
{
    // Function-local static avoids SIOF with other static DLogCategory instances
    static std::vector<DLogCategory*> s_categories;
    return s_categories;
}

DLogCategory* DLogCategory::FindByName(std::string_view name)
{
    auto& cats = GetAllCategories();
    const auto it = std::ranges::find_if(cats, [name](const DLogCategory* c) { return c->GetName() == name; });
    return it != cats.end() ? *it : nullptr;
}

DLogCategory::DLogCategory(std::string_view name, ELogLevel defaultLevel)
    : m_name(name)
    , m_level(defaultLevel)
{
    DELTA_ENSURE_MSG(!m_name.empty(), "DLogCategory constructed with empty name");

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
    DELTA_ENSURE_MSG(!sinks.empty(), "DLogCategory::ReinitializeWithSinks called with empty sink list ({})", m_name);

    spdlog::drop(m_name);
    m_logger = std::make_shared<spdlog::logger>(m_name, sinks.begin(), sinks.end());
    m_logger->set_level(ToSpdlogLevel(m_level));
    spdlog::register_logger(m_logger);

    DLOG(LogCore, ELogLevel::Verbose, "DLogCategory '{}' re-initialized with {} sinks", m_name, sinks.size());
}

DELTA_ENGINE_NS_END
