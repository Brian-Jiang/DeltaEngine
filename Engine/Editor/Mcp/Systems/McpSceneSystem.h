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
    nlohmann::json QueryGetPosition(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryGetRotation(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryGetScale(EditorCore&, const nlohmann::json& params);

    nlohmann::json CommandCreateGameObject(EditorCore&, const nlohmann::json& params);
    nlohmann::json CommandDeleteGameObject(EditorCore&, const nlohmann::json& params);
    nlohmann::json CommandDuplicateGameObject(EditorCore&, const nlohmann::json& params);
    nlohmann::json CommandReparentSceneComponent(EditorCore&, const nlohmann::json& params);
    nlohmann::json CommandCreateComponent(EditorCore&, const nlohmann::json& params);
    nlohmann::json CommandDeleteComponent(EditorCore&, const nlohmann::json& params);
    nlohmann::json CommandSetPosition(EditorCore&, const nlohmann::json& params);
    nlohmann::json CommandSetRotation(EditorCore&, const nlohmann::json& params);
    nlohmann::json CommandSetScale(EditorCore&, const nlohmann::json& params);
};

DELTA_ENGINE_NS_END
