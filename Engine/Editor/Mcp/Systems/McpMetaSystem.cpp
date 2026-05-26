#include "McpMetaSystem.h"

#include "Editor/EditorCore.h"
#include "Mcp/McpRegistry.h"
#include "Runtime/IO/IOManager.h"

#include <filesystem>
#include <fstream>

using namespace DeltaEngine;

DEFINE_LOG_CATEGORY(DeltaEngine::LogMcpMeta);

void McpMetaSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterQuery("meta", "list_operations",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryListOperations(c, p); });
    registry.RegisterQuery("meta", "describe_operations",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryDescribeOperations(c, p); });
    registry.RegisterQuery("meta", "capabilities",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryCapabilities(c, p); });
}

void McpMetaSystem::EnsureSchemasLoaded()
{
    if (m_schemasLoaded)
        return;

    m_schemasLoaded = true;
    m_systemSchemas = nlohmann::json::object();

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
        {
            DLOG(LogMcpMeta, ELogLevel::Warning,
                 "Failed to open MCP schema file for read (path='{}'): expected readable regular file",
                 entry.path().string());
            continue;
        }

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
    }

    DLOG(LogMcpMeta, ELogLevel::Log,
         "Loaded MCP schemas: {} systems",
         m_systemSchemas.size());
}

nlohmann::json McpMetaSystem::QueryListOperations(EditorCore& c, const nlohmann::json&)
{
    McpRegistry* reg = c.GetMcpRegistry();
    if (!reg)
        return {{"ok", false}, {"error", "MCP registry not initialized"}};

    nlohmann::json systems = nlohmann::json::object();
    for (auto& sysName : reg->GetSystemNames())
    {
        systems[sysName] = {
            {"queries",  reg->GetQueryNames(sysName)},
            {"commands", reg->GetCommandNames(sysName)}
        };
    }

    return {
        {"ok", true},
        {"systems", systems}
    };
}

nlohmann::json McpMetaSystem::QueryDescribeOperations(EditorCore&, const nlohmann::json& params)
{
    EnsureSchemasLoaded();

    nlohmann::json queriesResult = nlohmann::json::object();
    nlohmann::json cmdsResult = nlohmann::json::object();
    std::vector<std::string> warnings;

    auto requested = params.value("targets", nlohmann::json::array());

    for (auto& entry : requested)
    {
        if (entry.contains("system") && entry.contains("command"))
        {
            std::string sys = entry["system"].get<std::string>();
            std::string cmd = entry["command"].get<std::string>();
            std::string key = sys + "/" + cmd;

            if (m_systemSchemas.contains(sys) &&
                m_systemSchemas[sys].contains("commands") &&
                m_systemSchemas[sys]["commands"].contains(cmd))
            {
                cmdsResult[key] = m_systemSchemas[sys]["commands"][cmd];
            }
            else
            {
                warnings.push_back("Unknown command: " + key);
            }
        }
        else if (entry.contains("system") && entry.contains("query"))
        {
            std::string sys = entry["system"].get<std::string>();
            std::string q = entry["query"].get<std::string>();
            std::string key = sys + "/" + q;

            if (m_systemSchemas.contains(sys) &&
                m_systemSchemas[sys].contains("queries") &&
                m_systemSchemas[sys]["queries"].contains(q))
            {
                queriesResult[key] = m_systemSchemas[sys]["queries"][q];
            }
            else
            {
                warnings.push_back("Unknown query: " + key);
            }
        }
    }

    nlohmann::json result = {
        {"ok", true},
        {"queries", queriesResult},
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
            {"systems", filtered}
        };
    }

    return {
        {"ok", true},
        {"systems", m_systemSchemas}
    };
}
