#pragma once

#include "EditorIncludes.h"

#include "Editor/Mcp/IMcpSystem.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class McpPostProcessSystem : public IMcpSystem
{
public:
    std::string_view GetSystemName() const override { return "post_process"; }
    void RegisterTools(McpRegistry& registry) override;

private:
    nlohmann::json CommandAddPass(EditorCore& core, const nlohmann::json& params);
    nlohmann::json CommandRemovePass(EditorCore& core, const nlohmann::json& params);
};

DELTA_ENGINE_NS_END
