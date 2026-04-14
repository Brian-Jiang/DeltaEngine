#include "McpViewportSystem.h"

#include "Editor/EditorCore.h"
#include "Editor/EditorMain.h"
#include "Editor/EditorViewportCamera.h"
#include "Editor/EditorRenderManager.h"
#include "Editor/EditorWindows/EditorWindow_Viewport.h"
#include "Mcp/McpRegistry.h"
#include "Runtime/Core/DWorld.h"
#include "Runtime/Core/GameObject.h"
#include "Runtime/Core/SceneComponent.h"

#include <DirectXCollision.h>
#include <DirectXMath.h>

#include "SimpleMath.h"

#include <algorithm>
#include <cmath>

using namespace DeltaEngine;
using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{

static nlohmann::json MakeError(const std::string& msg)
{
    return { {"ok", false}, {"error", msg} };
}

static void GetSceneRenderDims(float& outW, float& outH)
{
    if (g_editor)
    {
        UINT w = 0, h = 0;
        g_editor->GetSceneRenderSize(w, h);
        outW = static_cast<float>(w);
        outH = static_cast<float>(h);
        if (outW < 1.f || outH < 1.f)
        {
            outW = 1280.f;
            outH = 720.f;
        }
        return;
    }
    outW = 1280.f;
    outH = 720.f;
}

static void GetActiveCamera(EditorViewportCamera& out, EditorWindow_Viewport** outVp = nullptr)
{
    if (outVp)
        *outVp = nullptr;
    if (g_editor)
    {
        if (auto* vp = g_editor->GetEditorWindow<EditorWindow_Viewport>())
        {
            out = vp->GetPreviewCamera();
            if (outVp)
                *outVp = vp;
            return;
        }
    }
    std::vector<EditorViewportCamera> cams;
    if (LoadViewportCameras(cams) && !cams.empty())
    {
        out = cams[0];
        return;
    }
    out = EditorViewportCamera{};
}

static void ApplySetViewportCamera(const EditorViewportCamera& cam)
{
    if (g_editor)
    {
        if (auto* vp = g_editor->GetEditorWindow<EditorWindow_Viewport>())
        {
            vp->SetPreviewCamera(cam);
            return;
        }
    }
    std::vector<EditorViewportCamera> cams;
    LoadViewportCameras(cams);
    if (cams.empty())
        cams.resize(1);
    cams[0] = cam;
    SaveViewportCameras(cams);
}

static void MergeCameraParams(EditorViewportCamera& cam, const nlohmann::json& params)
{
    if (params.contains("position") && params["position"].is_array() && params["position"].size() >= 3)
    {
        const auto& a = params["position"];
        cam.position.x = a[0].get<float>();
        cam.position.y = a[1].get<float>();
        cam.position.z = a[2].get<float>();
    }
    if (params.contains("rotation") && params["rotation"].is_array())
    {
        const auto& a = params["rotation"];
        if (a.size() == 4)
        {
            cam.rotation.x = a[0].get<float>();
            cam.rotation.y = a[1].get<float>();
            cam.rotation.z = a[2].get<float>();
            cam.rotation.w = a[3].get<float>();
        }
        else if (a.size() == 3)
        {
            Quaternion q = Quaternion::CreateFromYawPitchRoll(
                a[1].get<float>(), a[0].get<float>(), a[2].get<float>());
            cam.rotation.x = q.x;
            cam.rotation.y = q.y;
            cam.rotation.z = q.z;
            cam.rotation.w = q.w;
        }
    }
    if (params.contains("fov"))
        cam.fov = std::clamp(params["fov"].get<float>(), 10.f, 170.f);
}

} // namespace

void McpViewportSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("viewport", "camera",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryCamera(c, p); });
    registry.RegisterOperation("viewport", "render_settings",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryRenderSettings(c, p); });
    registry.RegisterOperation("viewport", "visible_objects",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryVisibleObjects(c, p); });
    registry.RegisterOperation("viewport", "SetViewportCamera",
        [this](EditorCore& c, const nlohmann::json& p) { return CommandSetViewportCamera(c, p); });
}

nlohmann::json McpViewportSystem::QueryCamera(EditorCore&, const nlohmann::json&)
{
    EditorViewportCamera cam;
    GetActiveCamera(cam);

    float w, h;
    GetSceneRenderDims(w, h);

    nlohmann::json pos = nlohmann::json::array();
    pos.push_back(cam.position.x);
    pos.push_back(cam.position.y);
    pos.push_back(cam.position.z);

    nlohmann::json rot = nlohmann::json::array();
    rot.push_back(cam.rotation.x);
    rot.push_back(cam.rotation.y);
    rot.push_back(cam.rotation.z);
    rot.push_back(cam.rotation.w);

    return {
        {"ok", true},
        {"position", std::move(pos)},
        {"rotation", std::move(rot)},
        {"fov", cam.fov},
        {"near_plane", cam.nearPlane},
        {"far_plane", cam.farPlane},
        {"aspect", cam.GetAspectRatio(w, h)},
    };
}

nlohmann::json McpViewportSystem::QueryRenderSettings(EditorCore&, const nlohmann::json&)
{
    nlohmann::json j;
    j["ok"] = true;

    if (!g_editor)
    {
        j["note"] = "Editor UI not active; swap chain and scene render settings unavailable.";
        return j;
    }

    UINT sw = 0, sh = 0;
    g_editor->GetSceneRenderSize(sw, sh);
    j["scene_render_width"]  = sw;
    j["scene_render_height"] = sh;

    if (EditorRenderManager* rm = g_editor->GetRenderManager())
    {
        j["swapchain_width"]  = rm->GetWidth();
        j["swapchain_height"] = rm->GetHeight();
        j["vsync"]            = rm->IsVSync();
        j["fullscreen"]       = rm->IsFullscreen();
    }

    return j;
}

nlohmann::json McpViewportSystem::QueryVisibleObjects(EditorCore& core, const nlohmann::json& params)
{
    const bool includePartial = params.value("include_partial", true);

    DWorld* world = core.GetWorld();
    if (!world)
        return MakeError("no active world");

    EditorViewportCamera cam;
    GetActiveCamera(cam);

    float w, h;
    GetSceneRenderDims(w, h);

    CameraCB cb = cam.BuildCameraCB(w, h);
    XMMATRIX vp   = XMMatrixMultiply(cb.viewMatrix, cb.projectionMatrix);
    BoundingFrustum frustum;
    BoundingFrustum::CreateFromMatrix(frustum, vp);

    constexpr float kRadius = 1.f;

    nlohmann::json ids = nlohmann::json::array();

    for (GameObject* go : world->GetGameObjects())
    {
        SceneComponent* root = go->GetRootSceneComponent();
        if (!root)
            continue;

        Vector3 wp = root->GetWorldPosition();
        BoundingSphere sphere(XMFLOAT3(wp.x, wp.y, wp.z), kRadius);

        ContainmentType ct = frustum.Contains(sphere);
        const auto c = static_cast<uint32_t>(ct);
        constexpr uint32_t kDisjoint  = 0;
        constexpr uint32_t kContains  = 2;
        bool keep = includePartial ? (c != kDisjoint) : (c == kContains);
        if (!keep)
            continue;

        auto [assetId, objectId] = core.GetIdsForObject(go);
        ids.push_back(objectId.ToString());
    }

    return {
        {"ok", true},
        {"object_ids", std::move(ids)},
        {"culling_mode", "root_position_sphere"},
        {"culling_radius", kRadius},
    };
}

nlohmann::json McpViewportSystem::CommandSetViewportCamera(EditorCore&, const nlohmann::json& params)
{
    EditorViewportCamera cam;
    GetActiveCamera(cam);
    MergeCameraParams(cam, params);
    ApplySetViewportCamera(cam);
    return { {"ok", true} };
}
