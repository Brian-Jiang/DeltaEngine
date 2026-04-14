#include "McpQueryRouter.h"

#include "McpRegistry.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

static DLogCategory LogMcpRouter{ "LogMcpRouter", ELogLevel::Log };

McpQueryRouter::McpQueryRouter(EditorCore& core, McpRegistry& registry)
    : m_core(core)
    , m_registry(registry)
{
}

std::string McpQueryRouter::Route(const std::string& rawJson) const
{
    try
    {
        auto q = nlohmann::json::parse(rawJson);

        std::string type   = q.value("type", "query");
        std::string system = q.value("system", "");
        auto params = q.contains("params") ? q["params"]
                                           : nlohmann::json::object();

        if (type == "command")
        {
            std::string command = q.value("command", "");

            DLOG(LogMcpRouter, ELogLevel::Log,
                 "Received MCP command: system='{}', command='{}'", system, command);
            DLOG(LogMcpRouter, ELogLevel::Verbose,
                 "Routing MCP command: params={}", params.dump());

            if (system.empty() || command.empty())
            {
                return nlohmann::json{
                    {"ok", false},
                    {"error", "Command must include 'system' and 'command' fields"}
                }.dump();
            }

            return m_registry
                .Dispatch(system, command, m_core, params)
                .dump();
        }

        std::string operation = q.value("operation", q.value("query", ""));

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

        return m_registry
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
