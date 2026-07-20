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

// Executes an already-constructed editor command synchronously on the calling
// (main) thread and formats the standard MCP result payload:
//   { "ok": true,  "commandType": "<TypeName>", "objectId": "<createdId?>" }
//   { "ok": false, "commandType": "<TypeName>", "error": "Execute() returned false" }
DELTAEDITOR_API nlohmann::json RunEditorCommand(
    EditorCore& core,
    std::unique_ptr<EditorCommand> cmd);

// Returns the active scene's asset id, or a null AssetId when no scene is active.
DELTAEDITOR_API AssetId ActiveSceneAssetId(EditorCore& core);

DELTA_ENGINE_NS_END
