#include "McpProtocol.h"

#include "Editor/EditorCore.h"
#include "Runtime/Core/UUID.h"

DELTA_ENGINE_NS_BEGIN

std::string ResolveRequestId(const nlohmann::json& envelope)
{
    if (envelope.contains("request_id") && envelope["request_id"].is_string())
    {
        const std::string id = envelope["request_id"].get<std::string>();
        if (!id.empty())
            return id;
    }
    return UUID::Generate().ToString();
}

nlohmann::json MakeAcceptResponse(
    const std::string& requestId,
    const std::string& commandName,
    nlohmann::json handlerResult)
{
    nlohmann::json accept;
    accept["phase"] = "accept";
    accept["request_id"] = requestId;

    for (auto it = handlerResult.begin(); it != handlerResult.end(); ++it)
        accept[it.key()] = it.value();

    if (!accept.contains("expects_result"))
    {
        if (!accept.value("ok", false))
            accept["expects_result"] = false;
        else if (accept.value("queued", false))
            accept["expects_result"] = true;
        else
            accept["expects_result"] = false;
    }

    if (!accept.contains("command") && !commandName.empty())
        accept["command"] = commandName;

    return accept;
}

nlohmann::json EnqueueMcpCommand(
    EditorCore& core,
    std::string_view system,
    const std::string& commandName,
    nlohmann::json params,
    bool expectsResult)
{
    nlohmann::json envelope;
    envelope["type"] = "command";
    envelope["system"] = system;
    envelope["command"] = commandName;
    envelope["params"] = std::move(params);

    core.EnqueueSerializedCommand(envelope.dump());
    return {
        {"ok", true},
        {"queued", true},
        {"command", commandName},
        {"expects_result", expectsResult}
    };
}

DELTA_ENGINE_NS_END
