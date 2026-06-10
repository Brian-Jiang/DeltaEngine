#include "Shared/GpuGraphicsFixture.h"

#include "Runtime/Graphics/DirectX/DirectX12Texture.h"
#include "Runtime/Graphics/Shadow/ShadowPassManager.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class GpuShadowInfrastructureTests : public GpuGraphicsFixture
{
};

TEST_F(GpuShadowInfrastructureTests, DefaultStateHasNoShadowResources)
{
    ShadowPassManager mgr;

    EXPECT_FALSE(mgr.ShadowResourcesReady());
    EXPECT_EQ(mgr.GetDirectionalAtlasTexture(), nullptr);
    EXPECT_EQ(mgr.GetSpotAtlasTexture(), nullptr);
    EXPECT_EQ(mgr.GetPointCubeArrayTexture(), nullptr);
    EXPECT_EQ(mgr.GetShadowDepthPSO(), nullptr);
}

TEST_F(GpuShadowInfrastructureTests, InitializeCreatesAtlasesAndShadowDepthPso)
{
    ShadowPassManager mgr;
    mgr.Initialize(*GetDevice());

    ASSERT_TRUE(mgr.ShadowResourcesReady());

    const auto directionalAtlas = mgr.GetDirectionalAtlasTexture();
    const auto spotAtlas = mgr.GetSpotAtlasTexture();
    const auto pointCubeArray = mgr.GetPointCubeArrayTexture();
    ASSERT_NE(directionalAtlas, nullptr);
    ASSERT_NE(spotAtlas, nullptr);
    ASSERT_NE(pointCubeArray, nullptr);

    const D3D12_RESOURCE_DESC directionalDesc = directionalAtlas->GetD3D12ResourceDesc();
    EXPECT_EQ(directionalDesc.Width, 4096u);
    EXPECT_EQ(directionalDesc.Height, 4096u);
    EXPECT_EQ(directionalDesc.Format, DXGI_FORMAT_R32_TYPELESS);
    EXPECT_TRUE(directionalAtlas->CheckDSVSupport());
    EXPECT_NE(directionalAtlas->GetShaderResourceView().ptr, 0u);
    EXPECT_NE(directionalAtlas->GetDepthStencilView().ptr, 0u);

    const D3D12_RESOURCE_DESC spotDesc = spotAtlas->GetD3D12ResourceDesc();
    EXPECT_EQ(spotDesc.Width, 4096u);
    EXPECT_EQ(spotDesc.Height, 4096u);
    EXPECT_EQ(spotDesc.Format, DXGI_FORMAT_R32_TYPELESS);
    EXPECT_TRUE(spotAtlas->CheckDSVSupport());
    EXPECT_NE(spotAtlas->GetShaderResourceView().ptr, 0u);
    EXPECT_NE(spotAtlas->GetDepthStencilView().ptr, 0u);

    const D3D12_RESOURCE_DESC pointDesc = pointCubeArray->GetD3D12ResourceDesc();
    EXPECT_EQ(pointDesc.Width, 512u);
    EXPECT_EQ(pointDesc.Height, 512u);
    EXPECT_EQ(pointDesc.DepthOrArraySize, 48u);
    EXPECT_EQ(pointDesc.Format, DXGI_FORMAT_R32_TYPELESS);
    EXPECT_TRUE(pointCubeArray->CheckDSVSupport());
    EXPECT_NE(pointCubeArray->GetShaderResourceView().ptr, 0u);
    EXPECT_NE(pointCubeArray->GetDepthStencilView().ptr, 0u);

    const ShadowDepthPSO* shadowDepthPso = mgr.GetShadowDepthPSO();
    ASSERT_NE(shadowDepthPso, nullptr);
    EXPECT_NE(shadowDepthPso->GetRootSignature(), nullptr);
    EXPECT_NE(shadowDepthPso->GetPSO2D(), nullptr);
    EXPECT_NE(shadowDepthPso->GetPSOCube(), nullptr);

    mgr.Shutdown();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuShadowInfrastructureTests, ShutdownReleasesAllResources)
{
    ShadowPassManager mgr;
    mgr.Initialize(*GetDevice());
    ASSERT_TRUE(mgr.ShadowResourcesReady());

    mgr.Shutdown();

    EXPECT_FALSE(mgr.ShadowResourcesReady());
    EXPECT_EQ(mgr.GetDirectionalAtlasTexture(), nullptr);
    EXPECT_EQ(mgr.GetSpotAtlasTexture(), nullptr);
    EXPECT_EQ(mgr.GetPointCubeArrayTexture(), nullptr);
    EXPECT_EQ(mgr.GetShadowDepthPSO(), nullptr);

    mgr.Shutdown();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuShadowInfrastructureTests, ReInitializeIsSafe)
{
    ShadowPassManager mgr;
    mgr.Initialize(*GetDevice());
    ASSERT_TRUE(mgr.ShadowResourcesReady());

    mgr.Shutdown();
    EXPECT_FALSE(mgr.ShadowResourcesReady());

    mgr.Initialize(*GetDevice());
    ASSERT_TRUE(mgr.ShadowResourcesReady());

    mgr.Shutdown();
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}
