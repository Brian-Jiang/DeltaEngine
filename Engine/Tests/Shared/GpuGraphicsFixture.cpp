#include "Shared/GpuGraphicsFixture.h"

#include <d3dx12.h>

namespace DeltaEngine::Tests
{

std::shared_ptr<Adapter> GpuGraphicsFixture::s_adapter;
std::shared_ptr<Device> GpuGraphicsFixture::s_device;
std::shared_ptr<RenderTarget> GpuGraphicsFixture::s_renderTarget;
CommandQueue* GpuGraphicsFixture::s_directQueue = nullptr;

void GpuGraphicsFixture::SetUpTestSuite()
{
    if (!IsD3D12DebugLayerAvailable())
        GTEST_SKIP() << "D3D12 debug layer unavailable";

    EnableDebugLayerForTests();

    if (!IsWarpAdapterAvailable())
        GTEST_SKIP() << "WARP adapter unavailable";

    s_adapter = Adapter::Create(DXGI_GPU_PREFERENCE_UNSPECIFIED, /*useWarp=*/true);
    ASSERT_NE(s_adapter, nullptr);

    s_device = Device::Create(s_adapter);
    ASSERT_NE(s_device, nullptr);

    DisableBreakOnSeverity(s_device->GetD3D12Device().Get());

    s_renderTarget = std::make_shared<RenderTarget>();
    const auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM, 64, 64, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    D3D12_CLEAR_VALUE colorClearValue = { colorDesc.Format, { 0.0f, 0.0f, 0.0f, 1.0f } };
    auto colorTexture = s_device->CreateTexture(colorDesc, &colorClearValue);
    ASSERT_NE(colorTexture, nullptr);
    s_renderTarget->AttachTexture(AttachmentPoint::Color0, colorTexture);

    s_directQueue = &s_device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    ASSERT_NE(s_directQueue, nullptr);
}

void GpuGraphicsFixture::TearDownTestSuite()
{
    if (s_device)
    {
        s_device->Flush();
        s_device->ReleaseStaleDescriptors(UINT64_MAX);
        AssertGpuValidationClean(s_device->GetD3D12Device().Get());
        CommandList::ClearTextureCache();
        s_device->ReportLiveDeviceObjects();
        Device::ReportLiveObjects();
    }

    s_renderTarget.reset();
    s_device.reset();
    s_adapter.reset();
    s_directQueue = nullptr;
}

void GpuGraphicsFixture::TearDown()
{
    if (s_device)
        AssertGpuValidationClean(s_device->GetD3D12Device().Get());
}

uint64_t GpuGraphicsFixture::SubmitAndWait(std::shared_ptr<CommandList> commandList)
{
    const uint64_t fenceValue = s_directQueue->ExecuteCommandList(commandList);
    s_device->SetFrameFenceValue(fenceValue);
    s_directQueue->WaitForFenceValue(fenceValue);
    AssertGpuValidationClean(s_device->GetD3D12Device().Get());
    return fenceValue;
}

} // namespace DeltaEngine::Tests
