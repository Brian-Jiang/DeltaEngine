#pragma once

#include "EditorIncludes.h"
#include "Mcp/IMcpSystem.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class McpSelectionSystem : public IMcpSystem
{
public:
    std::string_view GetSystemName() const override { return "selection"; }
    void RegisterTools(McpRegistry& registry) override;

private:
    nlohmann::json QueryCurrent(EditorCore&, const nlohmann::json& params);
};

DELTA_ENGINE_NS_END
