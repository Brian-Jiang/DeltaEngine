#include "Shared/GpuGraphicsFixture.h"

#include "Runtime/Graphics/DXGraphicsContext.h"
#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/Graphics/DirectX/RootSignature.h"
#include "Runtime/Graphics/Shadow/ShadowDepthPSO.h"

#include <gtest/gtest.h>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

class GpuRenderManagerInitTests : public GpuGraphicsFixture
{
};

TEST_F(GpuRenderManagerInitTests, CreateRenderManager_BuildsRootSignatureAndShadowPso)
{
    auto manager = CreateRenderManager();
    ASSERT_NE(manager, nullptr);

    const auto rootSignature = manager->GetRootSignature();
    ASSERT_NE(rootSignature, nullptr);
    EXPECT_NE(rootSignature->GetD3D12RootSignature().Get(), nullptr);

    const ShadowDepthPSO* shadowDepthPso = manager->GetShadowDepthPSO();
    ASSERT_NE(shadowDepthPso, nullptr);
    EXPECT_NE(shadowDepthPso->GetRootSignature(), nullptr);
    EXPECT_NE(shadowDepthPso->GetPSO2D(), nullptr);
    EXPECT_NE(shadowDepthPso->GetPSOCube(), nullptr);

    DestroyRenderManager(manager);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuRenderManagerInitTests, CreateRenderManager_ExposesGraphicsContext)
{
    auto manager = CreateRenderManager();
    ASSERT_NE(manager, nullptr);

    const auto contextA = manager->GetGraphicsContext();
    const auto contextB = manager->GetGraphicsContext();
    ASSERT_NE(contextA, nullptr);
    EXPECT_EQ(contextA, contextB);
    EXPECT_EQ(contextA->renderManager.get(), manager.get());
    EXPECT_EQ(contextA->device, GetDevice());
    EXPECT_EQ(contextA->commandList, nullptr);

    DestroyRenderManager(manager);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuRenderManagerInitTests, OnDestroy_ReleasesResourcesCleanly)
{
    auto manager = CreateRenderManager();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->GetRootSignature(), nullptr);
    ASSERT_NE(manager->GetShadowDepthPSO(), nullptr);
    ASSERT_NE(manager->GetGraphicsContext(), nullptr);

    DestroyRenderManager(manager);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_F(GpuRenderManagerInitTests, SecondCreateDestroyCycle_SucceedsWithSharedDevice)
{
    auto manager = CreateRenderManager();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->GetRootSignature(), nullptr);
    ASSERT_NE(manager->GetShadowDepthPSO(), nullptr);
    ASSERT_NE(manager->GetGraphicsContext(), nullptr);
    DestroyRenderManager(manager);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());

    manager = CreateRenderManager();
    ASSERT_NE(manager, nullptr);
    ASSERT_NE(manager->GetRootSignature(), nullptr);
    ASSERT_NE(manager->GetShadowDepthPSO(), nullptr);
    ASSERT_NE(manager->GetGraphicsContext(), nullptr);
    DestroyRenderManager(manager);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}
