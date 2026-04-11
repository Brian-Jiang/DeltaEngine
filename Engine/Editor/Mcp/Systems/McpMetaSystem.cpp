#include "McpMetaSystem.h"

#include "Commands/EditorCommandRegistry.h"
#include "Mcp/McpRegistry.h"
#include "Runtime/Logging/LogCategory.h"
#include "Runtime/IO/IOManager.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

using namespace DeltaEngine;

static DLogCategory LogMcpMeta{ "LogMcpMeta", ELogLevel::Log };

void McpMetaSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("meta", "list_operations",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryListOperations(c, p); });
    registry.RegisterOperation("meta", "describe_operations",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryDescribeOperations(c, p); });
    registry.RegisterOperation("meta", "capabilities",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryCapabilities(c, p); });
}

void McpMetaSystem::EnsureSchemasLoaded()
{
    if (m_schemasLoaded)
        return;

    m_schemasLoaded = true;
    m_systemSchemas = nlohmann::json::object();
    m_commandSchemas = nlohmann::json::object();

    namespace fs = std::filesystem;

    fs::path schemasDir = fs::path(IOManager::GetToolsFolder()) / "DeltaMCP" / "Schemas";

    std::error_code ec;
    if (!fs::is_directory(schemasDir, ec))
    {
        DLOG(LogMcpMeta, ELogLevel::Warning,
             "MCP schema directory not found: {}", schemasDir.string());
        return;
    }

    for (auto& entry : fs::directory_iterator(schemasDir, ec))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
            continue;

        std::ifstream file(entry.path());
        if (!file.is_open())
            continue;

        nlohmann::json data;
        try
        {
            data = nlohmann::json::parse(file);
        }
        catch (const nlohmann::json::parse_error& e)
        {
            DLOG(LogMcpMeta, ELogLevel::Warning,
                 "Failed to parse schema {}: {}", entry.path().string(), e.what());
            continue;
        }

        if (data.contains("system"))
            m_systemSchemas[data["system"].get<std::string>()] = data;

        if (data.contains("commands"))
        {
            for (auto& [key, val] : data["commands"].items())
                m_commandSchemas[key] = val;
        }
    }

    DLOG(LogMcpMeta, ELogLevel::Log,
         "Loaded MCP schemas: {} systems, {} commands",
         m_systemSchemas.size(), m_commandSchemas.size());
}

nlohmann::json McpMetaSystem::QueryListOperations(EditorCore&, const nlohmann::json&)
{
    auto& registry = McpRegistry::Get();

    nlohmann::json systems = nlohmann::json::object();
    for (auto& sysName : registry.GetSystemNames())
        systems[sysName] = registry.GetOperationNames(sysName);

    auto commandNames = EditorCommandRegistry::Get().GetCommandNames();

    return {
        {"ok", true},
        {"systems", systems},
        {"commands", commandNames}
    };
}

nlohmann::json McpMetaSystem::QueryDescribeOperations(EditorCore&, const nlohmann::json& params)
{
    EnsureSchemasLoaded();

    nlohmann::json opsResult = nlohmann::json::object();
    nlohmann::json cmdsResult = nlohmann::json::object();
    std::vector<std::string> warnings;

    auto requested = params.value("operations", nlohmann::json::array());

    for (auto& entry : requested)
    {
        if (entry.contains("command"))
        {
            std::string name = entry["command"].get<std::string>();
            if (m_commandSchemas.contains(name))
                cmdsResult[name] = m_commandSchemas[name];
            else
                warnings.push_back("Unknown command: " + name);
        }
        else if (entry.contains("system") && entry.contains("operation"))
        {
            std::string sys = entry["system"].get<std::string>();
            std::string op = entry["operation"].get<std::string>();
            std::string key = sys + "/" + op;

            if (m_systemSchemas.contains(sys) &&
                m_systemSchemas[sys].contains("operations") &&
                m_systemSchemas[sys]["operations"].contains(op))
            {
                opsResult[key] = m_systemSchemas[sys]["operations"][op];
            }
            else
            {
                warnings.push_back("Unknown operation: " + key);
            }
        }
    }

    nlohmann::json result = {
        {"ok", true},
        {"operations", opsResult},
        {"commands", cmdsResult}
    };

    if (!warnings.empty())
        result["warnings"] = warnings;

    return result;
}

nlohmann::json McpMetaSystem::QueryCapabilities(EditorCore&, const nlohmann::json& params)
{
    EnsureSchemasLoaded();

    std::string filter = params.value("system_filter", "");

    if (!filter.empty())
    {
        nlohmann::json filtered = nlohmann::json::object();
        if (m_systemSchemas.contains(filter))
            filtered[filter] = m_systemSchemas[filter];

        return {
            {"ok", true},
            {"systems", filtered},
            {"commands", m_commandSchemas}
        };
    }

    return {
        {"ok", true},
        {"systems", m_systemSchemas},
        {"commands", m_commandSchemas}
    };
}
