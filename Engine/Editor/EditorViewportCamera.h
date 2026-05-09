#pragma once

#include "EditorIncludes.h"

#include "Runtime/Graphics/Structures/Camera.h"

#include <DirectXMath.h>
#include <filesystem>
#include <vector>

DELTA_ENGINE_NS_BEGIN

/// Editor-only fly camera for a viewport window. Never touches scene state.
struct EditorViewportCamera
{
    float fov       = 60.f;     // degrees; clamped to [10, 170] in the UI
    float nearPlane = 0.1f;     // always > 0.001f
    float farPlane  = 10000.f;  // always > nearPlane

    DirectX::XMFLOAT3 position = { 0.f, 1.f, -5.f };
    DirectX::XMFLOAT4 rotation = { 0.f, 0.f, 0.f, 1.f }; // unit quaternion (x,y,z,w)

    float GetAspectRatio(float w, float h) const { return (h > 0.f) ? w / h : 1.f; }

    CameraCB BuildCameraCB(float w, float h) const;
    ActiveRenderCamera BuildActiveRenderCamera(float w, float h) const;
};

/// Persists viewport camera states to a JSON file at the given path (parent dirs created).
DELTAEDITOR_API void SaveViewportCamerasToPath(const std::vector<EditorViewportCamera>& cameras,
    const std::filesystem::path& path);

DELTAEDITOR_API bool LoadViewportCamerasFromPath(std::vector<EditorViewportCamera>& cameras,
    const std::filesystem::path& path);

/// Persists all viewport camera states to Intermediate/EditorState/viewport_cameras.json.
/// Creates the directory if it does not exist.
void SaveViewportCameras(const std::vector<EditorViewportCamera>& cameras);

/// Loads camera states from Intermediate/EditorState/viewport_cameras.json.
/// Returns false and leaves cameras unchanged if the file is missing or malformed.
bool LoadViewportCameras(std::vector<EditorViewportCamera>& cameras);

DELTA_ENGINE_NS_END
