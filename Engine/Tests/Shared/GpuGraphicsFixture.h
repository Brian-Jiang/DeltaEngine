#pragma once

#include "Shared/GpuD3D12Validation.h"

#include "Runtime/Graphics/DirectX/Adapter.h"
#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"

#include <gtest/gtest.h>
#include <memory>

namespace DeltaEngine::Tests
{

class DXRenderManager;

class GpuGraphicsFixture : public ::testing::Test
{
protected:
    static void SetUpTestSuite();
    static void TearDownTestSuite();
    void TearDown() override;

    static std::shared_ptr<Device> GetDevice() { return s_device; }
    static std::shared_ptr<RenderTarget> GetRenderTarget() { return s_renderTarget; }
    static CommandQueue& GetDirectQueue() { return *s_directQueue; }
    static uint64_t SubmitAndWait(std::shared_ptr<CommandList> commandList);
    static std::shared_ptr<DXRenderManager> CreateRenderManager();
    static void DestroyRenderManager(std::shared_ptr<DXRenderManager>& manager);

    static std::shared_ptr<Adapter> s_adapter;
    static std::shared_ptr<Device> s_device;
    static std::shared_ptr<RenderTarget> s_renderTarget;
    static CommandQueue* s_directQueue;
};

} // namespace DeltaEngine::Tests
