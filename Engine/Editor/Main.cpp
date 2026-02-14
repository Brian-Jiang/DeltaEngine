#include <iostream>

#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DXRenderManager.h"

 #include "imgui.h"
 #include "backends/imgui_impl_sdl3.h"
 #include "backends/imgui_impl_dx12.h"

using namespace std;
using namespace DeltaEngine;

void DescriptorAllocate(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
{
    auto renderManager = static_cast<DXRenderManager*>(info->UserData);
    auto device = renderManager->GetDevice();
    auto allocation = device->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    //auto allocation = descriptorAllocator->Allocate(1);
    *out_cpu_desc_handle = allocation.GetDescriptorHandle();
    *out_gpu_desc_handle = allocation.GetGpuHandle();
}

void DescriptorFree(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
{
    //auto renderManager = static_cast<DXRenderManager*>(info->UserData);
    //auto device = renderManager->GetDevice();
    //device->FreeDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cpu_desc_handle, gpu_desc_handle);
}

int main()
{
	auto engine = new EngineMain();
    engine->Initialize();
    printf("Delta Engine Init");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::ShowDemoWindow();
    ImGui_ImplSDL3_InitForD3D(engine->GetWindow());

    std::shared_ptr<Device> device = engine->GetRenderManager()->GetDevice();
    CommandQueue& directCommandQueue = device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = device->GetD3D12Device().Get();
    initInfo.CommandQueue = directCommandQueue.GetD3D12CommandQueue().Get();
    initInfo.NumFramesInFlight = SwapChain::BufferCount;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.UserData = engine->GetRenderManager().get();
    initInfo.SrvDescriptorAllocFn = DescriptorAllocate;
    initInfo.SrvDescriptorFreeFn = DescriptorFree;
    
    ImGui_ImplDX12_Init(&initInfo);

    engine->StartMainLoop();
    auto returnCode = engine->exitCode;

    delete engine;

    return returnCode;
}