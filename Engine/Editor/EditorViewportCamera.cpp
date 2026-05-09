#include "Editor/EditorViewportCamera.h"

#include "Editor/EditorWindows/EditorWindowsLog.h"

#include "Runtime/IO/IOManager.h"

#include <DirectXMath.h>

#include <filesystem>
#include <fstream>
#include <exception>
#include <nlohmann/json.hpp>

using namespace DeltaEngine;
using namespace DirectX;

CameraCB EditorViewportCamera::BuildCameraCB(float w, float h) const
{
    XMVECTOR rot  = XMLoadFloat4(&rotation);
    XMMATRIX rotM = XMMatrixRotationQuaternion(rot);
    XMVECTOR fwd  = XMVector3Normalize(rotM.r[2]);
    XMVECTOR up   = XMVector3Normalize(rotM.r[1]);
    XMVECTOR pos  = XMLoadFloat3(&position);

    CameraCB cb{};
    cb.viewMatrix = XMMatrixTranspose(XMMatrixLookToLH(pos, fwd, up));
    cb.projectionMatrix = XMMatrixTranspose(XMMatrixPerspectiveFovLH(
        XMConvertToRadians(fov),
        GetAspectRatio(w, h),
        nearPlane,
        farPlane));
    cb.position = pos;
    return cb;
}

ActiveRenderCamera EditorViewportCamera::BuildActiveRenderCamera(float w, float h) const
{
    ActiveRenderCamera arc{};
    arc.cb = BuildCameraCB(w, h);
    arc.nearPlane = nearPlane;
    arc.farPlane = farPlane;
    arc.aspectRatio = GetAspectRatio(w, h);
    arc.fovY = XMConvertToRadians(fov);
    return arc;
}

static std::filesystem::path GetCameraStatePath()
{
    return IOManager::GetIntermediateFolder() / "EditorState" / "viewport_cameras.json";
}

void DeltaEngine::SaveViewportCamerasToPath(const std::vector<EditorViewportCamera>& cameras,
    const std::filesystem::path& path)
{
    std::filesystem::create_directories(path.parent_path());

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& cam : cameras)
    {
        nlohmann::json entry;
        entry["fov"]       = cam.fov;
        entry["nearPlane"] = cam.nearPlane;
        entry["farPlane"]  = cam.farPlane;
        entry["position"]  = { cam.position.x, cam.position.y, cam.position.z };
        entry["rotation"]  = { cam.rotation.x, cam.rotation.y, cam.rotation.z, cam.rotation.w };
        arr.push_back(entry);
    }

    std::ofstream file(path);
    if (!file.is_open())
    {
        DLOG(LogEditorWindows, ELogLevel::Error,
            "SaveViewportCamerasToPath failed: could not open '{}' for write (expected writable parent folders)",
            path.string());
        return;
    }
    file << arr.dump(2);
    if (!file.good())
    {
        DLOG(LogEditorWindows, ELogLevel::Error,
            "SaveViewportCamerasToPath failed: incomplete write to '{}' (expected full JSON body)",
            path.string());
    }
}

bool DeltaEngine::LoadViewportCamerasFromPath(std::vector<EditorViewportCamera>& cameras,
    const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
        return false;

    try
    {
        std::ifstream file(path);
        const nlohmann::json arr = nlohmann::json::parse(file);
        if (!arr.is_array())
        {
            DLOG(LogEditorWindows, ELogLevel::Error,
                "LoadViewportCamerasFromPath failed: root is not JSON array in '{}' (expected camera array)",
                path.string());
            return false;
        }

        std::vector<EditorViewportCamera> result;
        result.reserve(arr.size());
        for (const auto& entry : arr)
        {
            EditorViewportCamera cam;
            cam.fov       = entry.value("fov", cam.fov);
            cam.nearPlane = entry.value("nearPlane", cam.nearPlane);
            cam.farPlane  = entry.value("farPlane", cam.farPlane);

            if (entry.contains("position") && entry["position"].is_array() && entry["position"].size() == 3)
            {
                cam.position.x = entry["position"][0];
                cam.position.y = entry["position"][1];
                cam.position.z = entry["position"][2];
            }
            if (entry.contains("rotation") && entry["rotation"].is_array() && entry["rotation"].size() == 4)
            {
                cam.rotation.x = entry["rotation"][0];
                cam.rotation.y = entry["rotation"][1];
                cam.rotation.z = entry["rotation"][2];
                cam.rotation.w = entry["rotation"][3];
            }
            result.push_back(cam);
        }

        cameras = std::move(result);
        return true;
    }
    catch (const std::exception& ex)
    {
        DLOG(LogEditorWindows, ELogLevel::Error,
            "LoadViewportCamerasFromPath failed parsing '{}': {} (expected valid JSON camera array)",
            path.string(), ex.what());
        return false;
    }
    catch (...)
    {
        DLOG(LogEditorWindows, ELogLevel::Error,
            "LoadViewportCamerasFromPath unknown exception reading '{}' (expected valid JSON)",
            path.string());
        return false;
    }
}

void DeltaEngine::SaveViewportCameras(const std::vector<EditorViewportCamera>& cameras)
{
    SaveViewportCamerasToPath(cameras, GetCameraStatePath());
}

bool DeltaEngine::LoadViewportCameras(std::vector<EditorViewportCamera>& cameras)
{
    return LoadViewportCamerasFromPath(cameras, GetCameraStatePath());
}