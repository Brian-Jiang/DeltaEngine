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

    void RegisterOperation(std::string_view system,
                           std::string_view operation,
                           McpOperationHandler handler);

    DELTAEDITOR_API void InitializeAll(EditorCore& core);

    DELTAEDITOR_API nlohmann::json Dispatch(const std::string& system,
                                            const std::string& operation,
                                            EditorCore& core,
                                            const nlohmann::json& params) const;

    DELTAEDITOR_API std::vector<std::string> GetSystemNames() const;
    DELTAEDITOR_API std::vector<std::string> GetOperationNames(const std::string& system) const;
    DELTAEDITOR_API bool HasOperation(const std::string& system, const std::string& op) const;

private:
    std::unordered_map<
        std::string,
        std::unordered_map<std::string, McpOperationHandler>
    > m_handlers;

    std::vector<std::unique_ptr<IMcpSystem>> m_systems;
    bool m_initialized = false;
};

DELTA_ENGINE_NS_END
