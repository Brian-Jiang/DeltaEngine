#pragma once

#include "EditorIncludes.h"
#include "Mcp/IMcpSystem.h"

#include <nlohmann/json.hpp>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class McpAssetsSystem : public IMcpSystem
{
public:
    std::string_view GetSystemName() const override { return "assets"; }
    void RegisterTools(McpRegistry& registry) override;

private:
    nlohmann::json QueryList(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryGet(EditorCore&, const nlohmann::json& params);
    nlohmann::json QuerySearch(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryFolderTree(EditorCore&, const nlohmann::json& params);
    nlohmann::json QueryUsages(EditorCore&, const nlohmann::json& params);
};

DELTA_ENGINE_NS_END
