#pragma once

#include "EditorIncludes.h"
#include "Mcp/IMcpSystem.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class McpViewportSystem : public IMcpSystem
{
public:
    std::string_view GetSystemName() const override { return "viewport"; }
    void RegisterTools(McpRegistry& registry) override;

private:
    nlohmann::json QueryCamera(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryRenderSettings(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryVisibleObjects(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryRaycast(EditorCore&, const nlohmann::json& params);
};

DELTA_ENGINE_NS_END
