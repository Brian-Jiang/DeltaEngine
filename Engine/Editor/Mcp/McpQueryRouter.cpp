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

        // Correlation id echoed back on the single response for this request.
        const std::string requestId = q.value("request_id", "");
        auto finish = [&requestId](nlohmann::json response) -> std::string {
            if (!requestId.empty())
                response["request_id"] = requestId;
            return response.dump();
        };

        if (type == "command")
        {
            std::string command = q.value("command", "");

            DLOG(LogMcpRouter, ELogLevel::Log,
                 "Received MCP command: system='{}', command='{}'", system, command);
            DLOG(LogMcpRouter, ELogLevel::Verbose,
                 "Routing MCP command: params={}", params.dump());

            if (system.empty() || command.empty())
                return finish({{"ok", false},
                               {"error", "Command must include 'system' and 'command' fields"}});

            // Commands execute synchronously on the main thread and return their
            // real result in this single response.
            return finish(m_registry.DispatchCommand(system, command, m_core, params));
        }

        std::string query = q.value("query", "");

        DLOG(LogMcpRouter, ELogLevel::Log,
             "Received MCP query: system='{}', query='{}'", system, query);
        DLOG(LogMcpRouter, ELogLevel::Verbose,
             "Routing MCP query: params={}", params.dump());

        if (system.empty() || query.empty())
        {
            return finish({
                {"ok", false},
                {"error", "Query must include 'system' and 'query' fields"},
                {"hint", "Call editor('meta','list_operations') to see all "
                         "available systems, queries, and commands"}
            });
        }

        return finish(m_registry.DispatchQuery(system, query, m_core, params));
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
