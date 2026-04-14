#pragma once

#include "EditorIncludes.h"

#include <nlohmann/json.hpp>

#include <string>

DELTA_ENGINE_NS_BEGIN

class EditorCore;
class McpRegistry;

class McpQueryRouter
{
public:
    McpQueryRouter(EditorCore& core, McpRegistry& registry);

    DELTAEDITOR_API std::string Route(const std::string& rawJson) const;

private:
    EditorCore& m_core;
    McpRegistry& m_registry;
};

DELTA_ENGINE_NS_END
