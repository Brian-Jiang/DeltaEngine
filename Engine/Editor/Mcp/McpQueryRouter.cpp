#include "McpQueryRouter.h"

#include "McpProtocol.h"
#include "McpRegistry.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

DEFINE_LOG_CATEGORY(LogMcpRouter)

McpQueryRouter::McpQueryRouter(EditorCore& core, McpRegistry& registry)
    : m_core(core)
    , m_registry(registry)
{
}

std::string McpQueryRouter::Route(const std::string& rawJson) const
{
    if (rawJson.empty())
    {
        DLOG(LogMcpRouter, ELogLevel::Warning, "Empty MCP envelope (expected non-empty UTF-8 JSON)");
        return nlohmann::json{
            {"ok", false},
            {"error", "Empty MCP request body (expected UTF-8 JSON object)"}}
            .dump();
    }

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
            const std::string requestId = ResolveRequestId(q);

            DLOG(LogMcpRouter, ELogLevel::Log,
                 "Received MCP command: system='{}', command='{}'", system, command);
            DLOG(LogMcpRouter, ELogLevel::Verbose,
                 "Routing MCP command: params={}", params.dump());

            if (system.empty() || command.empty())
            {
                return MakeAcceptResponse(
                    requestId,
                    command,
                    nlohmann::json{
                        {"ok", false},
                        {"error", "Command must include 'system' and 'command' fields"}
                    })
                    .dump();
            }

            return MakeAcceptResponse(
                requestId,
                command,
                m_registry.DispatchCommand(system, command, m_core, params))
                .dump();
        }

        std::string query = q.value("query", "");

        DLOG(LogMcpRouter, ELogLevel::Log,
             "Received MCP query: system='{}', query='{}'", system, query);
        DLOG(LogMcpRouter, ELogLevel::Verbose,
             "Routing MCP query: params={}", params.dump());

        if (system.empty() || query.empty())
        {
            return nlohmann::json{
                {"ok", false},
                {"error", "Query must include 'system' and 'query' fields"},
                {"hint", "Call editor('meta','list_operations') to see all "
                         "available systems, queries, and commands"}
            }.dump();
        }

        return m_registry
            .DispatchQuery(system, query, m_core, params)
            .dump();
    }
    catch (const nlohmann::json::parse_error& e)
    {
        DLOG(LogMcpRouter, ELogLevel::Warning,
             "JSON parse error routing MCP envelope: {} input_bytes={}",
             e.what(),
             rawJson.size());
        return nlohmann::json{
            {"ok", false},
            {"error", std::string("JSON parse error: ") + e.what()}
        }.dump();
    }
    catch (const std::exception& e)
    {
        DLOG(LogMcpRouter, ELogLevel::Warning,
             "Exception routing MCP envelope: {} input_bytes={}",
             e.what(),
             rawJson.size());
        return nlohmann::json{{"ok", false}, {"error", e.what()}}.dump();
    }
}

DELTA_ENGINE_NS_END
