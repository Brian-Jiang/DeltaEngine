#pragma once

#include "EditorIncludes.h"
#include "Mcp/IMcpSystem.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class McpCommonSystem : public IMcpSystem
{
public:
    std::string_view GetSystemName() const override { return "common"; }
    void RegisterTools(McpRegistry& registry) override;

private:
    nlohmann::json CommandRenameObject(EditorCore&, const nlohmann::json& params);
    nlohmann::json CommandSetProperty(EditorCore&, const nlohmann::json& params);
    nlohmann::json CommandSaveProject(EditorCore&, const nlohmann::json& params);
};

DELTA_ENGINE_NS_END
