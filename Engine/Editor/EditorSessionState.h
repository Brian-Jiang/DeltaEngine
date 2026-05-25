#pragma once

#include "EditorIncludes.h"

#include <filesystem>
#include <string>

DELTA_ENGINE_NS_BEGIN

/// Editor-only preferences that persist between sessions.
struct EditorSessionState
{
    /** Absolute path to the last scene the user had open. Empty means no preference. */
    std::string lastScenePath;
};

DELTAEDITOR_API void SaveEditorSessionStateToPath(const EditorSessionState& state,
    const std::filesystem::path& path);

DELTAEDITOR_API bool LoadEditorSessionStateFromPath(EditorSessionState& state,
    const std::filesystem::path& path);

/// Persists session state to Intermediate/EditorState/editor_session.json.
DELTAEDITOR_API void SaveEditorSessionState(const EditorSessionState& state);

/// Loads session state from Intermediate/EditorState/editor_session.json.
/// Returns a default-initialised state if the file is missing or malformed.
DELTAEDITOR_API EditorSessionState LoadEditorSessionState();

DELTA_ENGINE_NS_END
