#include "Editor/EditorSessionState.h"

#include "Editor/EditorCore.h"

#include "Runtime/IO/IOManager.h"

#include <nlohmann/json.hpp>

#include <exception>
#include <fstream>

using namespace DeltaEngine;

static std::filesystem::path GetSessionStatePath()
{
    return IOManager::GetEditorStateFolder() / "editor_session.json";
}

void DeltaEngine::SaveEditorSessionStateToPath(const EditorSessionState& state,
    const std::filesystem::path& path)
{
    std::filesystem::create_directories(path.parent_path());

    nlohmann::json root;
    root["lastScenePath"] = state.lastScenePath;

    std::ofstream file(path);
    if (!file.is_open())
    {
        DLOG(LogEditorCore, ELogLevel::Error,
            "SaveEditorSessionStateToPath failed: could not open '{}' for write",
            path.string());
        return;
    }
    file << root.dump(2);
    if (!file.good())
    {
        DLOG(LogEditorCore, ELogLevel::Error,
            "SaveEditorSessionStateToPath failed: incomplete write to '{}'",
            path.string());
    }
}

bool DeltaEngine::LoadEditorSessionStateFromPath(EditorSessionState& state,
    const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
        return false;

    try
    {
        std::ifstream file(path);
        const nlohmann::json root = nlohmann::json::parse(file);
        if (!root.is_object())
        {
            DLOG(LogEditorCore, ELogLevel::Error,
                "LoadEditorSessionStateFromPath failed: root is not a JSON object in '{}'",
                path.string());
            return false;
        }

        EditorSessionState parsed;
        if (root.contains("lastScenePath") && root["lastScenePath"].is_string())
            parsed.lastScenePath = root["lastScenePath"].get<std::string>();

        state = std::move(parsed);
        return true;
    }
    catch (const std::exception& ex)
    {
        DLOG(LogEditorCore, ELogLevel::Error,
            "LoadEditorSessionStateFromPath failed parsing '{}': {}",
            path.string(), ex.what());
        return false;
    }
}

void DeltaEngine::SaveEditorSessionState(const EditorSessionState& state)
{
    SaveEditorSessionStateToPath(state, GetSessionStatePath());
}

EditorSessionState DeltaEngine::LoadEditorSessionState()
{
    EditorSessionState state;
    LoadEditorSessionStateFromPath(state, GetSessionStatePath());
    return state;
}
