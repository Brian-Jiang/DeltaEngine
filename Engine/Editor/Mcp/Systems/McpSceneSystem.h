#pragma once

#include "EditorIncludes.h"
#include "Mcp/IMcpSystem.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class McpSceneSystem : public IMcpSystem
{
public:
    std::string_view GetSystemName() const override { return "scene"; }
    void RegisterTools(McpRegistry& registry) override;

private:
    nlohmann::json QueryGameObjects(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryGameObject(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryHierarchy(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryComponent(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryComponentsOnObject(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryFindByProperty(EditorCore&, const nlohmann::json& params);
};

DELTA_ENGINE_NS_END
