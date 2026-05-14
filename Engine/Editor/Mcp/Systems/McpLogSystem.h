#pragma once

#include "EditorIncludes.h"
#include "Mcp/IMcpSystem.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class McpLogSystem : public IMcpSystem
{
public:
    std::string_view GetSystemName() const override { return "log"; }
    void RegisterTools(McpRegistry& registry) override;

private:
    nlohmann::json QueryFileLocation(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryRead(EditorCore&, const nlohmann::json& params);
};

DELTA_ENGINE_NS_END
