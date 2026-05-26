#pragma once

#include "EditorIncludes.h"
#include "Mcp/IMcpSystem.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

DECLARE_LOG_CATEGORY(LogMcpMeta)

class EditorCore;

class McpMetaSystem : public IMcpSystem
{
public:
    std::string_view GetSystemName() const override { return "meta"; }
    void RegisterTools(McpRegistry& registry) override;

private:
    nlohmann::json QueryListOperations(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryDescribeOperations(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryCapabilities(EditorCore&, const nlohmann::json& params);

    void EnsureSchemasLoaded();

    nlohmann::json m_systemSchemas;
    bool m_schemasLoaded = false;
};

DELTA_ENGINE_NS_END
