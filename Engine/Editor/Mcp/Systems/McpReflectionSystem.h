#pragma once

#include "EditorIncludes.h"
#include "Mcp/IMcpSystem.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class McpReflectionSystem : public IMcpSystem
{
public:
    std::string_view GetSystemName() const override { return "reflection"; }
    void RegisterTools(McpRegistry& registry) override;

private:
    nlohmann::json QueryClasses(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryClassSchema(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryInheritanceChain(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryFindClassesWithProperty(EditorCore&, const nlohmann::json& params);
};

DELTA_ENGINE_NS_END
