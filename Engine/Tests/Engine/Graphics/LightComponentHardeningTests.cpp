#include "Runtime/Graphics/Light/DirectionalLight.h"
#include "Runtime/Graphics/Light/PointLight.h"
#include "Runtime/Graphics/Light/SpotLight.h"

#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/RenderProxy/DirectionalLightRenderProxy.h"
#include "Runtime/Graphics/RenderProxy/PointLightRenderProxy.h"
#include "Runtime/Graphics/RenderProxy/SpotLightRenderProxy.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

#include <DirectXMath.h>
#include <gtest/gtest.h>

#include <memory>

using namespace DeltaEngine;
using namespace DirectX;

TEST(LightComponentHardeningTests, DirectionalLight_DefaultConstruction_HasRenderProxy)
{
    // Arrange / Act
    DirectionalLight* light = CreateDObject<DirectionalLight>();
    ASSERT_NE(light, nullptr);

    // Assert
    EXPECT_NE(light->GetRenderProxy(), nullptr);

    GetReflectionRegistry().DestroyObject(light);
}

TEST(LightComponentHardeningTests, DirectionalLight_PreGatherDrawCalls_NullContext_IsNoop)
{
    // Arrange
    DirectionalLight* light = CreateDObject<DirectionalLight>();
    ASSERT_NE(light, nullptr);

    // Act / Assert (no crash)
    light->PreGatherDrawCalls(nullptr);
    SUCCEED();

    GetReflectionRegistry().DestroyObject(light);
}

TEST(LightComponentHardeningTests, DirectionalLight_UpdateParameters_NegativeIntensity_Rejected)
{
    // Arrange
    DirectionalLight* light = CreateDObject<DirectionalLight>();
    ASSERT_NE(light, nullptr);
    auto ctx = std::make_shared<DXGraphicsContext>();

    // Act: invalid intensity should be rejected; PreGatherDrawCalls then uses
    // the default intensity (1.0) to populate the proxy.
    light->UpdateParameters(XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f),
        XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), -2.0f);
    light->PreGatherDrawCalls(ctx);

    // Assert
    ASSERT_EQ(ctx->directionalLights.size(), 1u);
    EXPECT_FLOAT_EQ(ctx->directionalLights[0].intensity, 1.0f);

    GetReflectionRegistry().DestroyObject(light);
}

TEST(LightComponentHardeningTests, PointLight_DefaultConstruction_HasRenderProxy)
{
    // Arrange / Act
    PointLight* light = CreateDObject<PointLight>();
    ASSERT_NE(light, nullptr);

    // Assert
    EXPECT_NE(light->GetRenderProxy(), nullptr);

    GetReflectionRegistry().DestroyObject(light);
}

TEST(LightComponentHardeningTests, PointLight_PreGatherDrawCalls_NullContext_IsNoop)
{
    // Arrange
    PointLight* light = CreateDObject<PointLight>();
    ASSERT_NE(light, nullptr);

    // Act / Assert
    light->PreGatherDrawCalls(nullptr);
    SUCCEED();

    GetReflectionRegistry().DestroyObject(light);
}

TEST(LightComponentHardeningTests, PointLight_UpdateParameters_ZeroRange_Rejected)
{
    // Arrange
    PointLight* light = CreateDObject<PointLight>();
    ASSERT_NE(light, nullptr);
    auto ctx = std::make_shared<DXGraphicsContext>();

    // Act: range=0 must be rejected; default range (10) remains.
    light->UpdateParameters(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 0.0f);
    light->PreGatherDrawCalls(ctx);

    // Assert
    ASSERT_EQ(ctx->pointLights.size(), 1u);
    EXPECT_FLOAT_EQ(ctx->pointLights[0].range, 10.0f);

    GetReflectionRegistry().DestroyObject(light);
}

TEST(LightComponentHardeningTests, PointLight_UpdateParameters_NegativeIntensity_Rejected)
{
    // Arrange
    PointLight* light = CreateDObject<PointLight>();
    ASSERT_NE(light, nullptr);
    auto ctx = std::make_shared<DXGraphicsContext>();

    // Act
    light->UpdateParameters(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), -1.0f, 5.0f);
    light->PreGatherDrawCalls(ctx);

    // Assert: defaults preserved (intensity 1.0, range 10.0)
    ASSERT_EQ(ctx->pointLights.size(), 1u);
    EXPECT_FLOAT_EQ(ctx->pointLights[0].intensity, 1.0f);
    EXPECT_FLOAT_EQ(ctx->pointLights[0].range, 10.0f);

    GetReflectionRegistry().DestroyObject(light);
}

TEST(LightComponentHardeningTests, SpotLight_DefaultConstruction_HasRenderProxy)
{
    // Arrange / Act
    SpotLight* light = CreateDObject<SpotLight>();
    ASSERT_NE(light, nullptr);

    // Assert
    EXPECT_NE(light->GetRenderProxy(), nullptr);

    GetReflectionRegistry().DestroyObject(light);
}

TEST(LightComponentHardeningTests, SpotLight_PreGatherDrawCalls_NullContext_IsNoop)
{
    // Arrange
    SpotLight* light = CreateDObject<SpotLight>();
    ASSERT_NE(light, nullptr);

    // Act / Assert
    light->PreGatherDrawCalls(nullptr);
    SUCCEED();

    GetReflectionRegistry().DestroyObject(light);
}

TEST(LightComponentHardeningTests, SpotLight_UpdateParameters_InnerGreaterThanOuter_Rejected)
{
    // Arrange
    SpotLight* light = CreateDObject<SpotLight>();
    ASSERT_NE(light, nullptr);
    auto ctx = std::make_shared<DXGraphicsContext>();

    // Act: inner (1.5) > outer (0.5) is invalid; defaults remain.
    light->UpdateParameters(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 5.0f, 1.5f, 0.5f);
    light->PreGatherDrawCalls(ctx);

    // Assert: default range 10.0 preserved (proxy populated from defaults).
    ASSERT_EQ(ctx->spotLights.size(), 1u);
    EXPECT_FLOAT_EQ(ctx->spotLights[0].range, 10.0f);

    GetReflectionRegistry().DestroyObject(light);
}

TEST(LightComponentHardeningTests, SpotLight_UpdateParameters_ZeroOuterCone_Rejected)
{
    // Arrange
    SpotLight* light = CreateDObject<SpotLight>();
    ASSERT_NE(light, nullptr);
    auto ctx = std::make_shared<DXGraphicsContext>();

    // Act
    light->UpdateParameters(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 5.0f, 0.0f, 0.0f);
    light->PreGatherDrawCalls(ctx);

    // Assert: defaults preserved
    ASSERT_EQ(ctx->spotLights.size(), 1u);
    EXPECT_FLOAT_EQ(ctx->spotLights[0].range, 10.0f);

    GetReflectionRegistry().DestroyObject(light);
}
