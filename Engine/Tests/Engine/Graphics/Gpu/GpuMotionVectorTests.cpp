#include "Shared/GpuReadback.h"
#include "Shared/GpuSceneBuilder.h"
#include "Shared/GpuTestAssetFixture.h"

#include "Runtime/Core/Camera.h"
#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/Graphics/Renderer/MeshRenderer.h"
#include "Runtime/Graphics/Structures/TemporalCameraState.h"
#include "Runtime/Graphics/Structures/Vertex.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include <DirectXMath.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <optional>
#include <string>

using namespace DirectX;
using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
constexpr float kZeroTolerance = 1e-4f;
constexpr float kMotionTolerance = 2e-3f;
constexpr uint32_t kWarmupFrames = 3;

XMFLOAT2 PixelCenterUv(const uint32_t x, const uint32_t y, const GpuReadbackImage& image)
{
    return { (static_cast<float>(x) + 0.5f) / static_cast<float>(image.width),
        (static_cast<float>(y) + 0.5f) / static_cast<float>(image.height) };
}

XMVECTOR UvToNdc(const XMFLOAT2 uv, const float z)
{
    return XMVectorSet(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f, z, 1.0f);
}

XMFLOAT2 ProjectToUv(FXMMATRIX viewProjection, FXMVECTOR worldPoint)
{
    const XMVECTOR ndc = XMVector3TransformCoord(worldPoint, viewProjection);
    return { XMVectorGetX(ndc) * 0.5f + 0.5f, 0.5f - XMVectorGetY(ndc) * 0.5f };
}

XMVECTOR Unproject(FXMMATRIX viewProjection, const XMFLOAT2 uv, const float ndcZ)
{
    const XMMATRIX inverse = XMMatrixInverse(nullptr, viewProjection);
    return XMVector3TransformCoord(UvToNdc(uv, ndcZ), inverse);
}

std::optional<XMVECTOR> IntersectSphere(FXMVECTOR origin, FXMVECTOR direction, FXMVECTOR center, const float radius)
{
    const XMVECTOR oc = XMVectorSubtract(origin, center);
    const float b = XMVectorGetX(XMVector3Dot(oc, direction));
    const float c = XMVectorGetX(XMVector3Dot(oc, oc)) - radius * radius;
    const float discriminant = b * b - c;
    if (discriminant < 0.0f)
        return std::nullopt;

    const float t = -b - std::sqrt(discriminant);
    if (t <= 0.0f)
        return std::nullopt;
    return XMVectorAdd(origin, XMVectorScale(direction, t));
}

/** Surface point of a sphere seen through the pixel centre at uv, in the frame described by viewProjection. */
std::optional<XMVECTOR> SphereSurfaceAtUv(FXMMATRIX viewProjection, const XMFLOAT2 uv, FXMVECTOR center, const float radius)
{
    const XMVECTOR nearPoint = Unproject(viewProjection, uv, 0.0f);
    const XMVECTOR farPoint = Unproject(viewProjection, uv, 1.0f);
    const XMVECTOR direction = XMVector3Normalize(XMVectorSubtract(farPoint, nearPoint));
    return IntersectSphere(nearPoint, direction, center, radius);
}

XMFLOAT2 Sub(const XMFLOAT2 a, const XMFLOAT2 b)
{
    return { a.x - b.x, a.y - b.y };
}

float Length(const GpuReadbackPixel& pixel)
{
    return std::sqrt(pixel.r * pixel.r + pixel.g * pixel.g);
}

void ExpectMotionNear(const GpuReadbackPixel& actual, const XMFLOAT2 expected, const float tolerance,
    const std::string& where)
{
    EXPECT_NEAR(actual.r, expected.x, tolerance) << where;
    EXPECT_NEAR(actual.g, expected.y, tolerance) << where;
}

DMesh* GetRendererMesh(MeshRenderer* renderer)
{
    DClass* cls = renderer ? renderer->GetClass() : nullptr;
    DProperty* meshProp = cls ? cls->FindPropertyByName("m_mesh") : nullptr;
    void* value = meshProp ? meshProp->GetValue(renderer) : nullptr;
    return value ? *static_cast<DMesh**>(value) : nullptr;
}

float MeshBoundingRadius(MeshRenderer* renderer)
{
    DMesh* mesh = GetRendererMesh(renderer);
    if (!mesh)
        return 0.0f;

    float radius = 0.0f;
    for (const auto& submesh : mesh->GetVertices())
    {
        for (const Vertex& vertex : submesh)
        {
            const XMVECTOR p = XMLoadFloat3(&vertex.position);
            radius = (std::max)(radius, XMVectorGetX(XMVector3Length(p)));
        }
    }
    const XMMATRIX world = renderer->GetWorldTransform();
    return radius * XMVectorGetX(XMVector3Length(world.r[0]));
}

/** Opaque sphere at world radius 1 (the shared source mesh has radius 100 and a Transparent material). */
MeshRenderer* AddUnitSphere(GpuSceneBuilder& builder, const char* name, const float x = 0.0f, const float y = 0.0f,
    const float z = 0.0f)
{
    MeshRenderer* sphere = builder.AddSphereMesh(name, x, y, z);
    const float meshRadius = MeshBoundingRadius(sphere);
    if (!sphere || meshRadius <= 0.0f)
        return nullptr;
    DMesh* mesh = GetRendererMesh(sphere);
    DMaterial* material = mesh ? mesh->GetMaterial(0) : nullptr;
    if (!material)
        return nullptr;
    material->SetRenderMode(ERenderMode::Opaque);
    const float scale = 1.0f / meshRadius;
    sphere->SetLocalScale(scale, scale, scale);
    return sphere;
}

std::pair<uint32_t, uint32_t> UvToPixel(const XMFLOAT2 uv, const GpuReadbackImage& image)
{
    const auto clampToImage = [](const float value, const uint32_t size)
    {
        const float pixel = std::floor(value * static_cast<float>(size));
        return static_cast<uint32_t>(std::clamp(pixel, 0.0f, static_cast<float>(size - 1)));
    };
    return { clampToImage(uv.x, image.width), clampToImage(uv.y, image.height) };
}
} // namespace

class GpuMotionVectorTests : public GpuTestAssetFixture, public ::testing::WithParamInterface<RenderPath>
{
protected:
    RenderPath GetInitialRenderPath() const override { return GetParam(); }

    /** Unjittered view-projection committed by the last rendered frame. */
    XMMATRIX LastViewProjection() { return GetRenderManager().GetTemporalCameraState().prevUnjitteredViewProjection; }

    void ReadMotion(GpuReadbackImage& image)
    {
        const std::shared_ptr<DirectX12Texture> motion = GetRenderManager().GetMotionVectorTexture();
        ASSERT_NE(motion, nullptr);
        EXPECT_EQ(motion->GetD3D12ResourceDesc().Format, kMotionVectorFormat);
        ASSERT_TRUE(ReadTexturePixels(*GetDevice(), GetDirectQueue(), motion, image));
        ASSERT_GT(image.width, 0u);
    }

    void BuildScene(GpuSceneBuilder& builder, const bool withSkybox)
    {
        // Skybox costs an IBL bake per test, so only the camera test uses it.
        if (withSkybox)
            ASSERT_TRUE(builder.AddDefaultSkybox());
        m_camera = builder.AddCamera();
        ASSERT_NE(m_camera, nullptr);
    }

    void StartTemporal(GpuSceneBuilder& builder)
    {
        builder.InitGpuResources();
        GetRenderManager().SetTemporalEnabled(true);
        RenderSceneFrames(kWarmupFrames);
    }

    void ExpectZeroEverywhere(const GpuReadbackImage& image)
    {
        for (uint32_t y = 0; y < image.height; ++y)
            for (uint32_t x = 0; x < image.width; ++x)
                ASSERT_LT(Length(image.At(x, y)), kZeroTolerance) << "x=" << x << " y=" << y;
    }

    Camera* m_camera = nullptr;
};

TEST_P(GpuMotionVectorTests, StaticThenRotatedThenTranslatedCamera_MotionMatchesProjectedDifference)
{
    // Deferred + skybox + GBuffer geometry faults inside WARP (device removed) independently of the
    // motion target, so the skybox is covered on the forward path only.
    const bool withSkybox = GetParam() == RenderPath::Forward;

    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NO_FATAL_FAILURE(BuildScene(builder, withSkybox));
    MeshRenderer* sphere = AddUnitSphere(builder, "Sphere");
    ASSERT_NE(sphere, nullptr);
    const float radius = MeshBoundingRadius(sphere);
    ASSERT_GT(radius, 0.0f);
    StartTemporal(builder);

    // Static scene + static camera with jitter active: no motion on sphere or sky.
    const XMFLOAT2 jitter = GetRenderManager().GetTemporalCameraState().currentJitter;
    ASSERT_TRUE(jitter.x != 0.0f || jitter.y != 0.0f);
    GpuReadbackImage image;
    ASSERT_NO_FATAL_FAILURE(ReadMotion(image));
    ASSERT_NO_FATAL_FAILURE(ExpectZeroEverywhere(image));

    // Pure yaw: motion is depth-independent, so sky and sphere share one formula.
    XMMATRIX prevViewProjection = LastViewProjection();
    m_camera->SetLocalRotation(XMQuaternionRotationRollPitchYaw(0.0f, XMConvertToRadians(2.0f), 0.0f));
    RenderSceneFrame();
    ASSERT_FALSE(GetRenderManager().GetTemporalCameraState().historyResetThisFrame);
    XMMATRIX viewProjection = LastViewProjection();
    ASSERT_NO_FATAL_FAILURE(ReadMotion(image));
    for (uint32_t y = 0; y < image.height; ++y)
    {
        for (uint32_t x = 0; x < image.width; ++x)
        {
            const XMFLOAT2 uv = PixelCenterUv(x, y, image);
            // Without a skybox the background is cleared to zero; check sphere interior pixels only.
            if (!withSkybox && !SphereSurfaceAtUv(viewProjection, uv, XMVectorZero(), radius * 0.9f).has_value())
                continue;
            const XMFLOAT2 expected = Sub(uv, ProjectToUv(prevViewProjection, Unproject(viewProjection, uv, 0.5f)));
            ExpectMotionNear(image.At(x, y), expected, kMotionTolerance,
                "yaw x=" + std::to_string(x) + " y=" + std::to_string(y));
        }
    }

    // Pure translation: sky stays still, sphere moves by its projected difference.
    prevViewProjection = viewProjection;
    m_camera->SetLocalPosition(0.3f, 0.0f, -8.0f);
    RenderSceneFrame();
    ASSERT_FALSE(GetRenderManager().GetTemporalCameraState().historyResetThisFrame);
    viewProjection = LastViewProjection();
    ASSERT_NO_FATAL_FAILURE(ReadMotion(image));

    const XMVECTOR center = XMVectorZero();
    const auto [px, py] = UvToPixel(ProjectToUv(viewProjection, center), image);
    const XMFLOAT2 uv = PixelCenterUv(px, py, image);
    const std::optional<XMVECTOR> surface = SphereSurfaceAtUv(viewProjection, uv, center, radius);
    ASSERT_TRUE(surface.has_value());
    const XMFLOAT2 expected = Sub(uv, ProjectToUv(prevViewProjection, *surface));
    EXPECT_GT(std::fabs(expected.x), 10.0f * kMotionTolerance);
    ExpectMotionNear(image.At(px, py), expected, kMotionTolerance, "sphere centre");

    const uint32_t right = image.width - 1;
    const uint32_t bottom = image.height - 1;
    for (const auto [x, y] : { std::pair{ 0u, 0u }, std::pair{ right, 0u }, std::pair{ 0u, bottom }, std::pair{ right, bottom } })
        EXPECT_LT(Length(image.At(x, y)), kZeroTolerance) << "sky corner x=" << x << " y=" << y;
}

TEST_P(GpuMotionVectorTests, OneObjectMoved_OnlyItsPixelsHaveMotion)
{
    constexpr float kOffsetX = 1.75f;
    const XMVECTOR delta = XMVectorSet(0.0f, 0.2f, 0.0f, 0.0f);

    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NO_FATAL_FAILURE(BuildScene(builder, false));
    MeshRenderer* staticSphere = AddUnitSphere(builder, "StaticSphere", -kOffsetX, 0.0f, 0.0f);
    MeshRenderer* movingSphere = AddUnitSphere(builder, "MovingSphere", kOffsetX, 0.0f, 0.0f);
    ASSERT_NE(staticSphere, nullptr);
    ASSERT_NE(movingSphere, nullptr);
    const float radius = MeshBoundingRadius(movingSphere);
    ASSERT_GT(radius, 0.0f);
    StartTemporal(builder);

    const XMVECTOR movedCenter = XMVectorAdd(XMVectorSet(kOffsetX, 0.0f, 0.0f, 1.0f), delta);
    movingSphere->SetLocalPosition(movedCenter);
    RenderSceneFrame();
    const XMMATRIX viewProjection = LastViewProjection();

    GpuReadbackImage image;
    ASSERT_NO_FATAL_FAILURE(ReadMotion(image));

    const auto [mx, my] = UvToPixel(ProjectToUv(viewProjection, movedCenter), image);
    const XMFLOAT2 movedUv = PixelCenterUv(mx, my, image);
    const std::optional<XMVECTOR> surface = SphereSurfaceAtUv(viewProjection, movedUv, movedCenter, radius);
    ASSERT_TRUE(surface.has_value());
    const XMFLOAT2 expected = Sub(movedUv, ProjectToUv(viewProjection, XMVectorSubtract(*surface, delta)));
    EXPECT_GT(std::fabs(expected.y), 10.0f * kMotionTolerance);
    ExpectMotionNear(image.At(mx, my), expected, kMotionTolerance, "moving sphere centre");

    const XMVECTOR staticCenter = XMVectorSet(-kOffsetX, 0.0f, 0.0f, 1.0f);
    const auto [sx, sy] = UvToPixel(ProjectToUv(viewProjection, staticCenter), image);
    EXPECT_LT(Length(image.At(sx, sy)), kZeroTolerance) << "static sphere centre";

    // Every pixel with motion lies inside the moving sphere's screen-space footprint.
    const XMFLOAT2 centerUv = ProjectToUv(viewProjection, movedCenter);
    const XMFLOAT2 edgeUv = ProjectToUv(viewProjection, XMVectorAdd(movedCenter, XMVectorSet(0.0f, radius, 0.0f, 0.0f)));
    const float footprintPixels = std::fabs(edgeUv.y - centerUv.y) * static_cast<float>(image.height) * 1.25f + 2.0f;
    for (uint32_t y = 0; y < image.height; ++y)
    {
        for (uint32_t x = 0; x < image.width; ++x)
        {
            if (Length(image.At(x, y)) < kZeroTolerance)
                continue;
            const float dx = static_cast<float>(x) + 0.5f - centerUv.x * static_cast<float>(image.width);
            const float dy = static_cast<float>(y) + 0.5f - centerUv.y * static_cast<float>(image.height);
            EXPECT_LE(std::sqrt(dx * dx + dy * dy), footprintPixels) << "unexpected motion at x=" << x << " y=" << y;
        }
    }
}

TEST_P(GpuMotionVectorTests, TransparentObjectMoved_WritesNoMotion)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NO_FATAL_FAILURE(BuildScene(builder, false));
    MeshRenderer* sphere = AddUnitSphere(builder, "GlassSphere");
    ASSERT_NE(sphere, nullptr);
    DMesh* mesh = GetRendererMesh(sphere);
    ASSERT_NE(mesh, nullptr);
    DMaterial* material = mesh->GetMaterial(0);
    ASSERT_NE(material, nullptr);
    material->SetRenderMode(ERenderMode::Transparent);
    StartTemporal(builder);

    sphere->SetLocalPosition(0.4f, 0.0f, 0.0f);
    RenderSceneFrame();

    GpuReadbackImage image;
    ASSERT_NO_FATAL_FAILURE(ReadMotion(image));
    ExpectZeroEverywhere(image);
}

INSTANTIATE_TEST_SUITE_P(RenderPaths, GpuMotionVectorTests,
    ::testing::Values(RenderPath::Forward, RenderPath::Deferred),
    [](const ::testing::TestParamInfo<RenderPath>& info)
    {
        return info.param == RenderPath::Forward ? std::string("Forward") : std::string("Deferred");
    });
