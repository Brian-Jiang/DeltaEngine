#include "McpRegistry.h"

#include "Mcp/Systems/McpAssetsSystem.h"
#include "Mcp/Systems/McpCommonSystem.h"
#include "Mcp/Systems/McpLogSystem.h"
#include "Mcp/Systems/McpMetaSystem.h"
#include "Mcp/Systems/McpProjectSystem.h"
#include "Mcp/Systems/McpReflectionSystem.h"
#include "Mcp/Systems/McpSceneSystem.h"
#include "Mcp/Systems/McpLightsSystem.h"
#include "Mcp/Systems/McpSelectionSystem.h"
#include "Mcp/Systems/McpUndoSystem.h"
#include "Mcp/Systems/McpViewportSystem.h"

using namespace DeltaEngine;

DEFINE_LOG_CATEGORY(DeltaEngine::LogMcpRegistry);

void McpRegistry::RegisterOperation(std::string_view system,
                                    std::string_view operation,
                                    McpOperationHandler handler)
{
    auto& ops        = m_handlers[std::string(system)];
    const std::string opStr(operation);
    DELTA_ASSERT_MSG(ops.find(opStr) == ops.end(),
                       "Duplicate MCP operation registration ({}/{})",
                       system,
                       operation);
    ops[opStr] = std::move(handler);
    DLOG(LogMcpRegistry, ELogLevel::VeryVerbose,
         "Registered MCP operation: {}/{}", system, operation);
}

void McpRegistry::InitializeAll(EditorCore& core)
{
    (void)core;
    DELTA_ASSERT_MSG(!m_initialized, "McpRegistry::InitializeAll called twice");

    m_systems.push_back(std::make_unique<McpSceneSystem>());
    m_systems.push_back(std::make_unique<McpSelectionSystem>());
    m_systems.push_back(std::make_unique<McpLightsSystem>());
    m_systems.push_back(std::make_unique<McpAssetsSystem>());
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

    size_t totalOps = 0;
    for (auto& [_, ops] : m_handlers)
        totalOps += ops.size();

    DLOG(LogMcpRegistry, ELogLevel::Log,
         "MCP registry initialized: {} systems, {} total operations",
         m_systems.size(), totalOps);
}

nlohmann::json McpRegistry::Dispatch(const std::string& system,
                                     const std::string& operation,
                                     EditorCore& core,
                                     const nlohmann::json& params) const
{
    auto sysIt = m_handlers.find(system);
    if (sysIt == m_handlers.end())
    {
        DLOG(LogMcpRegistry, ELogLevel::Warning,
             "Unknown MCP system '{}' for operation '{}': expected a name from list_operations",
             system,
             operation);
        return {{"ok", false},
                {"error", "Unknown system: " + system},
                {"hint", "System exists in JSON schema but has no registered "
                         "C++ handler. Check McpRegistry::InitializeAll() "
                         "was called and the system .cpp is compiled."}};
    }
    auto opIt = sysIt->second.find(operation);
    if (opIt == sysIt->second.end())
    {
        DLOG(LogMcpRegistry, ELogLevel::Warning,
             "Unknown MCP operation '{}' on system '{}': expected registered operation name",
             operation,
             system);
        return {{"ok", false},
                {"error", "Unknown operation '" + operation +
                          "' on system '" + system + "'"},
                {"hint", "Operation is in the JSON schema but not registered "
                         "in C++. The schema and implementation may be out of sync."}};
    }
    try
    {
        return opIt->second(core, params);
    }
    catch (const std::exception& e)
    {
        DLOG(LogMcpRegistry,
             ELogLevel::Error,
             "MCP handler exception for {}/{}: {}",
             system,
             operation,
             e.what());
        return {{"ok", false},
                {"error", std::string("Handler exception: ") + e.what()}};
    }
}

std::vector<std::string> McpRegistry::GetSystemNames() const
{
    std::vector<std::string> names;
    names.reserve(m_handlers.size());
    for (auto& [name, _] : m_handlers)
        names.push_back(name);
    return names;
}

std::vector<std::string> McpRegistry::GetOperationNames(const std::string& system) const
{
    auto it = m_handlers.find(system);
    if (it == m_handlers.end())
        return {};
    std::vector<std::string> names;
    for (auto& [name, _] : it->second)
        names.push_back(name);
    return names;
}

bool McpRegistry::HasOperation(const std::string& system,
                               const std::string& op) const
{
    auto it = m_handlers.find(system);
    if (it == m_handlers.end())
        return false;
    return it->second.count(op) > 0;
}
