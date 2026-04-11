#pragma once

#include "EditorIncludes.h"
#include "Mcp/IMcpSystem.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class McpProjectSystem : public IMcpSystem
{
public:
    std::string_view GetSystemName() const override { return "project"; }
    void RegisterTools(McpRegistry& registry) override;

private:
    nlohmann::json QueryInfo(EditorCore&, const nlohmann::json& params);
    nlohmann::json QuerySettings(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryOpenScenes(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryBuildState(EditorCore&, const nlohmann::json& params);
};

DELTA_ENGINE_NS_END
