#include "McpRegistry.h"

#include "Mcp/Systems/McpAssetsSystem.h"
#include "Mcp/Systems/McpCommonSystem.h"
#include "Mcp/Systems/McpLogSystem.h"
#include "Mcp/Systems/McpMetaSystem.h"
#include "Mcp/Systems/McpPostProcessSystem.h"
#include "Mcp/Systems/McpProjectSystem.h"
#include "Mcp/Systems/McpReflectionSystem.h"
#include "Mcp/Systems/McpSceneSystem.h"
#include "Mcp/Systems/McpLightsSystem.h"
#include "Mcp/Systems/McpSelectionSystem.h"
#include "Mcp/Systems/McpUndoSystem.h"
#include "Mcp/Systems/McpViewportSystem.h"

using namespace DeltaEngine;

DELTA_ENGINE_NS_BEGIN

DEFINE_LOG_CATEGORY(LogMcpRegistry)

DELTA_ENGINE_NS_END

void McpRegistry::RegisterQuery(std::string_view system,
                                std::string_view query,
                                McpOperationHandler handler)
{
    auto& qs = m_queryHandlers[std::string(system)];
    const std::string name(query);
    DELTA_ASSERT_MSG(qs.find(name) == qs.end(),
                       "Duplicate MCP query registration ({}/{})",
                       system,
                       query);
    qs[name] = std::move(handler);
    DLOG(LogMcpRegistry, ELogLevel::VeryVerbose,
         "Registered MCP query: {}/{}", system, query);
}

void McpRegistry::RegisterCommand(std::string_view system,
                                  std::string_view command,
                                  McpOperationHandler handler)
{
    auto& cs = m_commandHandlers[std::string(system)];
    const std::string name(command);
    DELTA_ASSERT_MSG(cs.find(name) == cs.end(),
                       "Duplicate MCP command registration ({}/{})",
                       system,
                       command);
    cs[name] = std::move(handler);
    DLOG(LogMcpRegistry, ELogLevel::VeryVerbose,
         "Registered MCP command: {}/{}", system, command);
}

void McpRegistry::InitializeAll(EditorCore& core)
{
    (void)core;
    DELTA_ASSERT_MSG(!m_initialized, "McpRegistry::InitializeAll called twice");

    m_systems.push_back(std::make_unique<McpSceneSystem>());
    m_systems.push_back(std::make_unique<McpSelectionSystem>());
    m_systems.push_back(std::make_unique<McpLightsSystem>());
    m_systems.push_back(std::make_unique<McpAssetsSystem>());
    m_systems.push_back(std::make_unique<McpPostProcessSystem>());
    m_systems.push_back(std::make_unique<McpViewportSystem>());
    m_systems.push_back(std::make_unique<McpReflectionSystem>());
    m_systems.push_back(std::make_unique<McpUndoSystem>());
    m_systems.push_back(std::make_unique<McpProjectSystem>());
    m_systems.push_back(std::make_unique<McpCommonSystem>());
    m_systems.push_back(std::make_unique<McpMetaSystem>());
    m_systems.push_back(std::make_unique<McpLogSystem>());

    for (auto& system : m_systems)
    {
        DLOG(LogMcpRegistry, ELogLevel::Log,
             "Registering MCP system: {}", system->GetSystemName());
        system->RegisterTools(*this);
    }

    m_initialized = true;

    size_t totalQueries = 0;
    for (auto& [_, qs] : m_queryHandlers)
        totalQueries += qs.size();
    size_t totalCommands = 0;
    for (auto& [_, cs] : m_commandHandlers)
        totalCommands += cs.size();

    DLOG(LogMcpRegistry, ELogLevel::Log,
         "MCP registry initialized: {} systems, {} queries, {} commands",
         m_systems.size(), totalQueries, totalCommands);
}

nlohmann::json McpRegistry::DispatchQuery(const std::string& system,
                                          const std::string& query,
                                          EditorCore& core,
                                          const nlohmann::json& params) const
{
    auto sysIt = m_queryHandlers.find(system);
    if (sysIt == m_queryHandlers.end())
    {
        DLOG(LogMcpRegistry, ELogLevel::Warning,
             "Unknown MCP system '{}' for query '{}': expected a name from list_operations",
             system,
             query);
        return {{"ok", false},
                {"error", "Unknown system: " + system},
                {"hint", "System exists in JSON schema but has no registered "
                         "C++ query handler. Check McpRegistry::InitializeAll() "
                         "was called and the system .cpp is compiled."}};
    }
    auto qIt = sysIt->second.find(query);
    if (qIt == sysIt->second.end())
    {
        DLOG(LogMcpRegistry, ELogLevel::Warning,
             "Unknown MCP query '{}' on system '{}': expected registered query name",
             query,
             system);
        return {{"ok", false},
                {"error", "Unknown query '" + query +
                          "' on system '" + system + "'"},
                {"hint", "Query is in the JSON schema but not registered "
                         "in C++. The schema and implementation may be out of sync."}};
    }
    try
    {
        return qIt->second(core, params);
    }
    catch (const std::exception& e)
    {
        DLOG(LogMcpRegistry,
             ELogLevel::Error,
             "MCP query handler exception for {}/{}: {}",
             system,
             query,
             e.what());
        return {{"ok", false},
                {"error", std::string("Handler exception: ") + e.what()}};
    }
}

nlohmann::json McpRegistry::DispatchCommand(const std::string& system,
                                            const std::string& command,
                                            EditorCore& core,
                                            const nlohmann::json& params) const
{
    auto sysIt = m_commandHandlers.find(system);
    if (sysIt == m_commandHandlers.end())
    {
        DLOG(LogMcpRegistry, ELogLevel::Warning,
             "Unknown MCP system '{}' for command '{}': expected a name from list_operations",
             system,
             command);
        return {{"ok", false},
                {"error", "Unknown system: " + system},
                {"hint", "System exists in JSON schema but has no registered "
                         "C++ command handler. Check McpRegistry::InitializeAll() "
                         "was called and the system .cpp is compiled."}};
    }
    auto cIt = sysIt->second.find(command);
    if (cIt == sysIt->second.end())
    {
        DLOG(LogMcpRegistry, ELogLevel::Warning,
             "Unknown MCP command '{}' on system '{}': expected registered command name",
             command,
             system);
        return {{"ok", false},
                {"error", "Unknown command '" + command +
                          "' on system '" + system + "'"},
                {"hint", "Command is in the JSON schema but not registered "
                         "in C++. The schema and implementation may be out of sync."}};
    }
    try
    {
        return cIt->second(core, params);
    }
    catch (const std::exception& e)
    {
        DLOG(LogMcpRegistry,
             ELogLevel::Error,
             "MCP command handler exception for {}/{}: {}",
             system,
             command,
             e.what());
        return {{"ok", false},
                {"error", std::string("Handler exception: ") + e.what()}};
    }
}

std::vector<std::string> McpRegistry::GetSystemNames() const
{
    std::vector<std::string> names;
    std::unordered_map<std::string, bool> seen;
    seen.reserve(m_queryHandlers.size() + m_commandHandlers.size());
    for (auto& [name, _] : m_queryHandlers)
    {
        if (!seen[name]) { seen[name] = true; names.push_back(name); }
    }
    for (auto& [name, _] : m_commandHandlers)
    {
        if (!seen[name]) { seen[name] = true; names.push_back(name); }
    }
    return names;
}

std::vector<std::string> McpRegistry::GetQueryNames(const std::string& system) const
{
    auto it = m_queryHandlers.find(system);
    if (it == m_queryHandlers.end())
        return {};
    std::vector<std::string> names;
    for (auto& [name, _] : it->second)
        names.push_back(name);
    return names;
}

std::vector<std::string> McpRegistry::GetCommandNames(const std::string& system) const
{
    auto it = m_commandHandlers.find(system);
    if (it == m_commandHandlers.end())
        return {};
    std::vector<std::string> names;
    for (auto& [name, _] : it->second)
        names.push_back(name);
    return names;
}

bool McpRegistry::HasQuery(const std::string& system,
                            const std::string& query) const
{
    auto it = m_queryHandlers.find(system);
    if (it == m_queryHandlers.end())
        return false;
    return it->second.count(query) > 0;
}

bool McpRegistry::HasCommand(const std::string& system,
                              const std::string& command) const
{
    auto it = m_commandHandlers.find(system);
    if (it == m_commandHandlers.end())
        return false;
    return it->second.count(command) > 0;
}
