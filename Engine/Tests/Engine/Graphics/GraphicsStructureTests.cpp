#include <windows.h>

#include "Runtime/Graphics/Structures/Camera.h"
#include "Runtime/Graphics/Structures/DeferredLightingRootParameterType.h"
#include "Runtime/Graphics/Structures/Light.h"
#include "Runtime/Graphics/Structures/RootParameterType.h"
#include "Runtime/Graphics/Structures/Vertex.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <type_traits>

using namespace DeltaEngine;

TEST(GraphicsStructureTests, ConstantBuffersRemainSixteenByteAligned)
{
    EXPECT_EQ(alignof(CameraCB), 16u);
    EXPECT_EQ(sizeof(CameraCB) % 16u, 0u);

    EXPECT_EQ(alignof(LightCB), 16u);
    EXPECT_EQ(sizeof(LightCB) % 16u, 0u);

    EXPECT_EQ(alignof(DirectionalLightBuffer), 16u);
    EXPECT_EQ(sizeof(DirectionalLightBuffer) % 16u, 0u);

    EXPECT_EQ(alignof(PointLightBuffer), 16u);
    EXPECT_EQ(sizeof(PointLightBuffer) % 16u, 0u);

    EXPECT_EQ(alignof(SpotLightBuffer), 16u);
    EXPECT_EQ(sizeof(SpotLightBuffer) % 16u, 0u);

    EXPECT_EQ(alignof(Light), 16u);
    EXPECT_EQ(sizeof(Light) % 16u, 0u);
}

TEST(GraphicsStructureTests, RootParameterOrderMatchesShaderSlots)
{
    EXPECT_EQ(static_cast<int>(RootParameterType::CameraCB), 0);
    EXPECT_EQ(static_cast<int>(RootParameterType::Texture), 1);
    EXPECT_EQ(static_cast<int>(RootParameterType::ObjectCB), 2);
    EXPECT_EQ(static_cast<int>(RootParameterType::LightCB), 3);
    EXPECT_EQ(static_cast<int>(RootParameterType::PointLights), 4);
    EXPECT_EQ(static_cast<int>(RootParameterType::SpotLights), 5);
    EXPECT_EQ(static_cast<int>(RootParameterType::DirectionalLights), 6);
    EXPECT_EQ(static_cast<int>(RootParameterType::MaterialCB), 7);
    EXPECT_EQ(static_cast<int>(RootParameterType::IBLTextures), 8);
    EXPECT_EQ(static_cast<int>(RootParameterType::ShadowMaps), 9);
    EXPECT_EQ(static_cast<int>(RootParameterType::ShadowCB), 10);
    EXPECT_EQ(static_cast<int>(RootParameterType::NumRootParameterTypes), 11);
}

TEST(GraphicsStructureTests, DeferredLightingRootParameterOrderMatchesShaderSlots)
{
    EXPECT_EQ(static_cast<int>(DeferredLightingRootParameterType::GBufferTextures), 0);
    EXPECT_EQ(static_cast<int>(DeferredLightingRootParameterType::CameraCB), 1);
    EXPECT_EQ(static_cast<int>(DeferredLightingRootParameterType::LightCB), 2);
    EXPECT_EQ(static_cast<int>(DeferredLightingRootParameterType::PointLights), 3);
    EXPECT_EQ(static_cast<int>(DeferredLightingRootParameterType::SpotLights), 4);
    EXPECT_EQ(static_cast<int>(DeferredLightingRootParameterType::DirectionalLights), 5);
    EXPECT_EQ(static_cast<int>(DeferredLightingRootParameterType::IBLTextures), 6);
    EXPECT_EQ(static_cast<int>(DeferredLightingRootParameterType::ShadowMaps), 7);
    EXPECT_EQ(static_cast<int>(DeferredLightingRootParameterType::ShadowCB), 8);
    EXPECT_EQ(static_cast<int>(DeferredLightingRootParameterType::NumRootParameterTypes), 9);
}

TEST(GraphicsStructureTests, VertexLayoutMatchesInputLayoutExpectations)
{
    static_assert(std::is_standard_layout_v<Vertex>);

    EXPECT_EQ(sizeof(Position), sizeof(float) * 2);
    EXPECT_EQ(sizeof(Color), 4u);
    EXPECT_EQ(sizeof(UV), sizeof(float) * 2);

    EXPECT_EQ(offsetof(Vertex, position), 0u);
    EXPECT_EQ(offsetof(Vertex, color), sizeof(DirectX::XMFLOAT3));
    EXPECT_EQ(offsetof(Vertex, normal), sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT4));
    EXPECT_EQ(offsetof(Vertex, uv), sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMFLOAT3) +
                                        sizeof(DirectX::XMFLOAT3));
}
