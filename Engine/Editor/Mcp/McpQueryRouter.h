#pragma once

#include "EditorIncludes.h"

#include <nlohmann/json.hpp>

#include <string>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

class McpQueryRouter
{
public:
    explicit McpQueryRouter(EditorCore& core);

    DELTAEDITOR_API std::string Route(const std::string& rawJson) const;

private:
    EditorCore& m_core;
};

DELTA_ENGINE_NS_END
