#include "Runtime/Graphics/RenderProxy/MeshRenderProxy.h"

#include "Runtime/Core/DMaterial.h"
#include "Runtime/Graphics/DXGraphicsContext.h"

#include <d3dx12.h>
#include <DirectXMath.h>
#include <gtest/gtest.h>

#include <memory>

using namespace DeltaEngine;

TEST(MeshRenderProxyTests, MeshRenderProxy_DefaultConstruction_HasZeroBuffers)
{
    // Arrange / Act
    MeshRenderProxy proxy;

    // Assert
    EXPECT_EQ(proxy.GetVertexCount(), 0u);
    EXPECT_EQ(proxy.GetIndexCount(), 0u);
}

TEST(MeshRenderProxyTests, MeshRenderProxy_SetMeshNull_ClearsBuffers)
{
    // Arrange
    MeshRenderProxy proxy;

    // Act
    proxy.SetMesh(nullptr);

    // Assert
    EXPECT_EQ(proxy.GetVertexCount(), 0u);
    EXPECT_EQ(proxy.GetIndexCount(), 0u);
}

TEST(MeshRenderProxyTests, MeshRenderProxy_UpdateWorldTransform_DoesNotCrash)
{
    // Arrange
    MeshRenderProxy proxy;
    const DirectX::XMMATRIX m = DirectX::XMMatrixTranslation(1.f, 2.f, 3.f);

    // Act
    proxy.UpdateWorldTransform(m);

    // Assert (no crash, no observable side effect to assert on directly)
    SUCCEED();
}

TEST(MeshRenderProxyTests, SubmeshContributesToShadowMap_NullMaterial_ReturnsTrue)
{
    // Arrange / Act
    const bool result = MeshRenderProxy::SubmeshContributesToShadowMap(nullptr);

    // Assert
    EXPECT_TRUE(result);
}

TEST(MeshRenderProxyTests, SubmeshContributesToShadowMap_OpaqueMaterial_ReturnsTrue)
{
    // Arrange
    DMaterial material;

    // Act
    const bool result = MeshRenderProxy::SubmeshContributesToShadowMap(&material);

    // Assert (default material has no AlphaBlend)
    EXPECT_TRUE(result);
}

TEST(MeshRenderProxyTests, SubmeshContributesToShadowMap_AlphaBlendMaterial_ReturnsFalse)
{
    // Arrange
    DMaterial material;
    CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC streamBlend(blendDesc);
    material.SetBlendState(streamBlend);

    // Act
    const bool result = MeshRenderProxy::SubmeshContributesToShadowMap(&material);

    // Assert
    EXPECT_FALSE(result);
}

TEST(MeshRenderProxyTests, SubmeshContributesToGBuffer_NullMaterial_ReturnsTrue)
{
    EXPECT_TRUE(MeshRenderProxy::SubmeshContributesToGBuffer(nullptr));
}

TEST(MeshRenderProxyTests, SubmeshContributesToGBuffer_OpaqueMaterial_ReturnsTrue)
{
    DMaterial material;
    EXPECT_TRUE(MeshRenderProxy::SubmeshContributesToGBuffer(&material));
}

TEST(MeshRenderProxyTests, SubmeshContributesToGBuffer_AlphaBlendMaterial_ReturnsFalse)
{
    DMaterial material;
    CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    material.SetBlendState(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC(blendDesc));

    EXPECT_FALSE(MeshRenderProxy::SubmeshContributesToGBuffer(&material));
}

TEST(MeshRenderProxyTests, SubmeshContributesToOpaquePass_NullMaterial_ReturnsTrue)
{
    EXPECT_TRUE(MeshRenderProxy::SubmeshContributesToOpaquePass(nullptr));
}

TEST(MeshRenderProxyTests, SubmeshContributesToOpaquePass_OpaqueMaterial_ReturnsTrue)
{
    DMaterial material;
    EXPECT_TRUE(MeshRenderProxy::SubmeshContributesToOpaquePass(&material));
}

TEST(MeshRenderProxyTests, SubmeshContributesToOpaquePass_AlphaBlendMaterial_ReturnsFalse)
{
    DMaterial material;
    CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    material.SetBlendState(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC(blendDesc));

    EXPECT_FALSE(MeshRenderProxy::SubmeshContributesToOpaquePass(&material));
}

TEST(MeshRenderProxyTests, SubmeshContributesToOpaquePass_TransparentRenderMode_ReturnsFalse)
{
    DMaterial material;
    material.SetRenderMode(ERenderMode::Transparent);

    EXPECT_FALSE(MeshRenderProxy::SubmeshContributesToOpaquePass(&material));
}

TEST(MeshRenderProxyTests, SubmeshContributesToTransparentPass_NullMaterial_ReturnsFalse)
{
    EXPECT_FALSE(MeshRenderProxy::SubmeshContributesToTransparentPass(nullptr));
}

TEST(MeshRenderProxyTests, SubmeshContributesToTransparentPass_OpaqueMaterial_ReturnsFalse)
{
    DMaterial material;
    EXPECT_FALSE(MeshRenderProxy::SubmeshContributesToTransparentPass(&material));
}

TEST(MeshRenderProxyTests, SubmeshContributesToTransparentPass_AlphaBlendMaterial_ReturnsTrue)
{
    DMaterial material;
    CD3DX12_BLEND_DESC blendDesc(D3D12_DEFAULT);
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    material.SetBlendState(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC(blendDesc));

    EXPECT_TRUE(MeshRenderProxy::SubmeshContributesToTransparentPass(&material));
}

TEST(MeshRenderProxyTests, SubmeshContributesToTransparentPass_TransparentRenderMode_ReturnsTrue)
{
    DMaterial material;
    material.SetRenderMode(ERenderMode::Transparent);

    EXPECT_TRUE(MeshRenderProxy::SubmeshContributesToTransparentPass(&material));
}

TEST(MeshRenderProxyTests, MeshRenderProxy_InitializeWithNullMesh_DoesNotCrash)
{
    // Arrange
    MeshRenderProxy proxy;
    auto ctx = std::make_shared<DXGraphicsContext>();

    // Act
    proxy.Initialize(ctx);

    // Assert (ENSURE fires + Warning logged; nothing else to verify)
    EXPECT_EQ(proxy.GetVertexCount(), 0u);
}

TEST(MeshRenderProxyTests, MeshRenderProxy_GatherDrawCallsWithNullMesh_IsNoop)
{
    // Arrange
    MeshRenderProxy proxy;
    auto ctx = std::make_shared<DXGraphicsContext>();

    // Act
    proxy.GatherDrawCalls(ctx);

    // Assert
    SUCCEED();
}
