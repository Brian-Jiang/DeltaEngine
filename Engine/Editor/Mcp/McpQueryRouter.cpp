#include "McpQueryRouter.h"

#include "McpRegistry.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

static DLogCategory LogMcpRouter{ "LogMcpRouter", ELogLevel::Log };

McpQueryRouter::McpQueryRouter(EditorCore& core)
    : m_core(core)
{
}

std::string McpQueryRouter::Route(const std::string& rawJson) const
{
    try
    {
        auto q = nlohmann::json::parse(rawJson);

        std::string system    = q.value("system", "");
        std::string operation = q.value("operation", q.value("query", ""));
        auto params = q.contains("params") ? q["params"]
                                           : nlohmann::json::object();

        DLOG(LogMcpRouter, ELogLevel::Log,
             "Received MCP query: system='{}', operation='{}'", system, operation);
        DLOG(LogMcpRouter, ELogLevel::Verbose,
             "Routing MCP query: params={}", params.dump());

        if (system.empty() || operation.empty())
        {
            return nlohmann::json{
                {"ok", false},
                {"error", "Query must include 'system' and 'operation' fields"},
                {"hint", "Call editor('meta','list_operations') to see all "
                         "available systems and operations"}
            }.dump();
        }

        return McpRegistry::Get()
            .Dispatch(system, operation, m_core, params)
            .dump();
    }
    catch (const nlohmann::json::parse_error& e)
    {
        return nlohmann::json{
            {"ok", false},
            {"error", std::string("JSON parse error: ") + e.what()}
        }.dump();
    }
    catch (const std::exception& e)
    {
        return nlohmann::json{{"ok", false}, {"error", e.what()}}.dump();
    }
}

DELTA_ENGINE_NS_END
