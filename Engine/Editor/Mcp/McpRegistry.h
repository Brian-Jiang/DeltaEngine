#pragma once

#include "EditorIncludes.h"

#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Mcp/IMcpSystem.h"

DELTA_ENGINE_NS_BEGIN

class EditorCore;

using McpOperationHandler =
    std::function<nlohmann::json(EditorCore&, const nlohmann::json& params)>;

DECLARE_LOG_CATEGORY(LogMcpRegistry)

class McpRegistry
{
public:
    DELTAEDITOR_API McpRegistry() = default;
    ~McpRegistry() = default;

    void RegisterQuery(std::string_view system,
                       std::string_view query,
                       McpOperationHandler handler);

    void RegisterCommand(std::string_view system,
                         std::string_view command,
                         McpOperationHandler handler);

    DELTAEDITOR_API void InitializeAll(EditorCore& core);

    DELTAEDITOR_API nlohmann::json DispatchQuery(const std::string& system,
                                                 const std::string& query,
                                                 EditorCore& core,
                                                 const nlohmann::json& params) const;

    DELTAEDITOR_API nlohmann::json DispatchCommand(const std::string& system,
                                                   const std::string& command,
                                                   EditorCore& core,
                                                   const nlohmann::json& params) const;

    DELTAEDITOR_API std::vector<std::string> GetSystemNames() const;
    DELTAEDITOR_API std::vector<std::string> GetQueryNames(const std::string& system) const;
    DELTAEDITOR_API std::vector<std::string> GetCommandNames(const std::string& system) const;
    DELTAEDITOR_API bool HasQuery(const std::string& system, const std::string& query) const;
    DELTAEDITOR_API bool HasCommand(const std::string& system, const std::string& command) const;

private:
    std::unordered_map<
        std::string,
        std::unordered_map<std::string, McpOperationHandler>
    > m_queryHandlers;

    std::unordered_map<
        std::string,
        std::unordered_map<std::string, McpOperationHandler>
    > m_commandHandlers;

    std::vector<std::unique_ptr<IMcpSystem>> m_systems;
    bool m_initialized = false;
};

DELTA_ENGINE_NS_END
