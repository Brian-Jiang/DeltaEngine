#pragma once

#include "EditorIncludes.h"

#include "Editor/Mcp/IMcpSystem.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class McpLightsSystem : public IMcpSystem
{
public:
    std::string_view GetSystemName() const override { return "lights"; }
    void RegisterTools(McpRegistry& registry) override;

private:
    nlohmann::json CommandSetIntensity(EditorCore& core, const nlohmann::json& params);
};

DELTA_ENGINE_NS_END
