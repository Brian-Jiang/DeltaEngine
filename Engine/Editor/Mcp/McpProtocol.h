#pragma once

#include "EditorIncludes.h"

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

DELTA_ENGINE_NS_BEGIN

class EditorCore;

DELTAEDITOR_API std::string ResolveRequestId(const nlohmann::json& envelope);

DELTAEDITOR_API nlohmann::json MakeAcceptResponse(
    const std::string& requestId,
    const std::string& commandName,
    nlohmann::json handlerResult);

DELTAEDITOR_API nlohmann::json MakeResultResponse(
    const std::string& requestId,
    nlohmann::json payload);

class McpRequestIdScope
{
public:
    DELTAEDITOR_API explicit McpRequestIdScope(EditorCore& core, std::string requestId);
    DELTAEDITOR_API ~McpRequestIdScope();

    McpRequestIdScope(const McpRequestIdScope&) = delete;
    McpRequestIdScope& operator=(const McpRequestIdScope&) = delete;

private:
    EditorCore& m_core;
};

DELTAEDITOR_API nlohmann::json EnqueueMcpCommand(
    EditorCore& core,
    std::string_view system,
    const std::string& commandName,
    nlohmann::json params,
    bool expectsResult = true);

DELTA_ENGINE_NS_END
