#pragma once

#include "EditorIncludes.h"

#include "Runtime/Core/UUID.h"

#include <nlohmann/json.hpp>

#include <memory>
#include <string>

DELTA_ENGINE_NS_BEGIN

class EditorCore;
class EditorCommand;

DELTAEDITOR_API nlohmann::json MakeMcpError(const std::string& message);

// Minimal success envelope: { "ok": true }. Handlers add their own fields.
DELTAEDITOR_API nlohmann::json MakeMcpOk();

// Runs an already-constructed editor command synchronously on the calling
// (main) thread via EditorCore's command manager. Returns true on success.
// On failure, fills 'outError' with { "ok": false, "commandType", "error" }.
// The command survives on the undo stack after a successful call, so a raw
// pointer captured before std::move stays valid for reading result data.
DELTAEDITOR_API bool ExecuteMcpCommand(
    EditorCore& core,
    std::unique_ptr<EditorCommand> cmd,
    nlohmann::json& outError);

// Returns the active scene's asset id, or a null AssetId when no scene is active.
DELTAEDITOR_API AssetId ActiveSceneAssetId(EditorCore& core);

DELTA_ENGINE_NS_END
