#include "Editor/EditorViewportCamera.h"

#include "Runtime/IO/IOManager.h"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace DeltaEngine;

static std::filesystem::path GetCameraStatePath()
{
    return std::filesystem::path(IOManager::GetIntermediateFolder()) / "EditorState" / "viewport_cameras.json";
}

void DeltaEngine::SaveViewportCameras(const std::vector<EditorViewportCamera>& cameras)
{
    const auto path = GetCameraStatePath();
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
    file << arr.dump(2);
}

bool DeltaEngine::LoadViewportCameras(std::vector<EditorViewportCamera>& cameras)
{
    const auto path = GetCameraStatePath();
    if (!std::filesystem::exists(path))
        return false;

    try
    {
        std::ifstream file(path);
        const nlohmann::json arr = nlohmann::json::parse(file);
        if (!arr.is_array())
            return false;

        std::vector<EditorViewportCamera> result;
        result.reserve(arr.size());
        for (const auto& entry : arr)
        {
            EditorViewportCamera cam;
            cam.fov       = entry.value("fov",       cam.fov);
            cam.nearPlane = entry.value("nearPlane",  cam.nearPlane);
            cam.farPlane  = entry.value("farPlane",   cam.farPlane);

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
    catch (...)
    {
        return false;
    }
}
