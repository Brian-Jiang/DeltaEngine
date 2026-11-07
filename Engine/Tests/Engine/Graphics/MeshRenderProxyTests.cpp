#include "Runtime/Graphics/RenderProxy/MeshRenderProxy.h"

#include "Runtime/Core/DMaterial.h"
#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/Structures/ObjectConstants.h"

#include <d3dx12.h>
#include <DirectXMath.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <memory>

using namespace DeltaEngine;
using namespace DirectX;

namespace
{
void ExpectMatrixEq(FXMMATRIX a, CXMMATRIX b)
{
    XMFLOAT4X4 fa;
    XMFLOAT4X4 fb;
    XMStoreFloat4x4(&fa, a);
    XMStoreFloat4x4(&fb, b);
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            EXPECT_FLOAT_EQ(fa.m[r][c], fb.m[r][c]) << "r=" << r << " c=" << c;
}
}

TEST(MeshRenderProxyTemporalTests, FirstFrame_PrevEqualsCurrent)
{
    MeshRenderProxy proxy;
    const XMMATRIX m = XMMatrixTranslation(1.f, 2.f, 3.f);
    proxy.UpdateWorldTransform(m);

    proxy.AdvanceTemporalFrame(1, false);

    ExpectMatrixEq(proxy.GetPrevWorldMatrix(), m);
}

TEST(MeshRenderProxyTemporalTests, NextFrame_PrevIsLastFrameMatrix)
{
    MeshRenderProxy proxy;
    const XMMATRIX a = XMMatrixTranslation(1.f, 0.f, 0.f);
    const XMMATRIX b = XMMatrixTranslation(2.f, 0.f, 0.f);
    proxy.UpdateWorldTransform(a);
    proxy.AdvanceTemporalFrame(1, false);

    proxy.UpdateWorldTransform(b);
    proxy.AdvanceTemporalFrame(2, false);

    ExpectMatrixEq(proxy.GetPrevWorldMatrix(), a);
}

TEST(MeshRenderProxyTemporalTests, Stationary_PrevCatchesUpToCurrent)
{
    MeshRenderProxy proxy;
    const XMMATRIX b = XMMatrixTranslation(2.f, 0.f, 0.f);
    proxy.UpdateWorldTransform(XMMatrixTranslation(1.f, 0.f, 0.f));
    proxy.AdvanceTemporalFrame(1, false);
    proxy.UpdateWorldTransform(b);
    proxy.AdvanceTemporalFrame(2, false);

    proxy.AdvanceTemporalFrame(3, false);

    ExpectMatrixEq(proxy.GetPrevWorldMatrix(), b);
}

TEST(MeshRenderProxyTemporalTests, SameFrameTwice_PrevUnchanged)
{
    MeshRenderProxy proxy;
    const XMMATRIX a = XMMatrixTranslation(1.f, 0.f, 0.f);
    proxy.UpdateWorldTransform(a);
    proxy.AdvanceTemporalFrame(1, false);
    proxy.UpdateWorldTransform(XMMatrixTranslation(2.f, 0.f, 0.f));
    proxy.AdvanceTemporalFrame(2, false);

    proxy.AdvanceTemporalFrame(2, true);

    ExpectMatrixEq(proxy.GetPrevWorldMatrix(), a);
}

TEST(MeshRenderProxyTemporalTests, GlobalHistoryReset_PrevEqualsCurrent)
{
    MeshRenderProxy proxy;
    const XMMATRIX b = XMMatrixTranslation(2.f, 0.f, 0.f);
    proxy.UpdateWorldTransform(XMMatrixTranslation(1.f, 0.f, 0.f));
    proxy.AdvanceTemporalFrame(1, false);
    proxy.UpdateWorldTransform(b);

    proxy.AdvanceTemporalFrame(2, true);

    ExpectMatrixEq(proxy.GetPrevWorldMatrix(), b);
}

TEST(MeshRenderProxyTemporalTests, Teleport_PrevEqualsCurrent)
{
    MeshRenderProxy proxy;
    const XMMATRIX b = XMMatrixTranslation(100.f, 0.f, 0.f);
    proxy.UpdateWorldTransform(XMMatrixTranslation(1.f, 0.f, 0.f));
    proxy.AdvanceTemporalFrame(1, false);
    proxy.UpdateWorldTransform(b, true);

    proxy.AdvanceTemporalFrame(2, false);

    ExpectMatrixEq(proxy.GetPrevWorldMatrix(), b);
}

TEST(MeshRenderProxyTemporalTests, RequestTemporalHistoryReset_PrevEqualsCurrent)
{
    MeshRenderProxy proxy;
    const XMMATRIX b = XMMatrixTranslation(2.f, 0.f, 0.f);
    proxy.UpdateWorldTransform(XMMatrixTranslation(1.f, 0.f, 0.f));
    proxy.AdvanceTemporalFrame(1, false);
    proxy.UpdateWorldTransform(b);
    proxy.RequestTemporalHistoryReset();

    proxy.AdvanceTemporalFrame(2, false);

    ExpectMatrixEq(proxy.GetPrevWorldMatrix(), b);
}

TEST(MeshRenderProxyTemporalTests, SkippedFrame_PrevEqualsCurrent)
{
    MeshRenderProxy proxy;
    const XMMATRIX b = XMMatrixTranslation(2.f, 0.f, 0.f);
    proxy.UpdateWorldTransform(XMMatrixTranslation(1.f, 0.f, 0.f));
    proxy.AdvanceTemporalFrame(1, false);
    proxy.UpdateWorldTransform(b);

    proxy.AdvanceTemporalFrame(3, false);

    ExpectMatrixEq(proxy.GetPrevWorldMatrix(), b);
}

TEST(MeshRenderProxyTemporalTests, ObjectCB_LayoutMatchesShaderPacking)
{
    EXPECT_EQ(sizeof(ObjectCB) % 16, 0u);
    EXPECT_EQ(alignof(ObjectCB), 16u);
    EXPECT_EQ(offsetof(ObjectCB, worldMatrix), 0u);
    EXPECT_EQ(offsetof(ObjectCB, prevWorldMatrix), 64u);
    EXPECT_EQ(offsetof(ObjectCB, color), 128u);
    EXPECT_EQ(offsetof(ObjectCB, useInstanceMatrix), 144u);
}

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
