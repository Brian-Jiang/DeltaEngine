#include "Editor/EditorViewportCamera.h"

#include "Editor/EditorWindows/EditorWindowsLog.h"

#include "Runtime/IO/IOManager.h"

#include <DirectXMath.h>

#include <cmath>
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
    PopulateInvViewProjection(cb);
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
    return IOManager::GetEditorStateFolder() / "viewport_cameras.json";
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

        auto readFinite = [&path](const nlohmann::json& src, const char* key, float fallback) -> float
        {
            if (!src.contains(key) || !src[key].is_number())
                return fallback;
            const float v = src[key].get<float>();
            if (!std::isfinite(v))
            {
                DLOG(LogEditorWindows, ELogLevel::Warning,
                     "LoadViewportCamerasFromPath: non-finite '{}' in '{}' (expected finite float); using default",
                     key, path.string());
                return fallback;
            }
            return v;
        };

        auto readFiniteAt = [&path](const nlohmann::json& arr, size_t idx, float fallback, const char* arrName) -> float
        {
            if (!arr[idx].is_number())
                return fallback;
            const float v = arr[idx].get<float>();
            if (!std::isfinite(v))
            {
                DLOG(LogEditorWindows, ELogLevel::Warning,
                     "LoadViewportCamerasFromPath: non-finite '{}[{}]' in '{}' (expected finite float); using default",
                     arrName, idx, path.string());
                return fallback;
            }
            return v;
        };

        std::vector<EditorViewportCamera> result;
        result.reserve(arr.size());
        for (const auto& entry : arr)
        {
            EditorViewportCamera cam;
            cam.fov       = readFinite(entry, "fov",       cam.fov);
            cam.nearPlane = readFinite(entry, "nearPlane", cam.nearPlane);
            cam.farPlane  = readFinite(entry, "farPlane",  cam.farPlane);

            if (entry.contains("position") && entry["position"].is_array() && entry["position"].size() == 3)
            {
                cam.position.x = readFiniteAt(entry["position"], 0, cam.position.x, "position");
                cam.position.y = readFiniteAt(entry["position"], 1, cam.position.y, "position");
                cam.position.z = readFiniteAt(entry["position"], 2, cam.position.z, "position");
            }
            if (entry.contains("rotation") && entry["rotation"].is_array() && entry["rotation"].size() == 4)
            {
                cam.rotation.x = readFiniteAt(entry["rotation"], 0, cam.rotation.x, "rotation");
                cam.rotation.y = readFiniteAt(entry["rotation"], 1, cam.rotation.y, "rotation");
                cam.rotation.z = readFiniteAt(entry["rotation"], 2, cam.rotation.z, "rotation");
                cam.rotation.w = readFiniteAt(entry["rotation"], 3, cam.rotation.w, "rotation");
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