#include <iostream>

#include "EngineIncludes.h"

#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/Graphics/DirectX/ImGuiSrvDescriptorAllocator.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_dx12.h"

using namespace std;
using namespace DeltaEngine;

static void DescriptorAllocate(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
{
    auto* allocator = static_cast<ImGuiSrvDescriptorAllocator*>(info->UserData);
    allocator->Alloc(out_cpu_desc_handle, out_gpu_desc_handle);
}

static void DescriptorFree(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle)
{
    auto* allocator = static_cast<ImGuiSrvDescriptorAllocator*>(info->UserData);
    allocator->Free(cpu_desc_handle, gpu_desc_handle);
}

int main()
{
	auto engine = new EngineMain();
    engine->Initialize();
    printf("Delta Engine Init");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //ImGui::ShowDemoWindow();
    ImGui_ImplSDL3_InitForD3D(engine->GetWindow());

    std::shared_ptr<DXRenderManager> renderManager = engine->GetRenderManager();
    std::shared_ptr<Device> device = renderManager->GetDevice();
    CommandQueue& directCommandQueue = device->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

    ImGuiSrvDescriptorAllocator srvAllocator;
    srvAllocator.Create(*device, device->CreateShaderVisibleSrvHeap(64));
    renderManager->m_cbvSrvUavDescriptorAllocator = &srvAllocator;

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = device->GetD3D12Device().Get();
    initInfo.CommandQueue = directCommandQueue.GetD3D12CommandQueue().Get();
    initInfo.NumFramesInFlight = SwapChain::BufferCount;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;
    initInfo.SrvDescriptorHeap = srvAllocator.GetHeap();
    initInfo.UserData = &srvAllocator;
    initInfo.SrvDescriptorAllocFn = DescriptorAllocate;
    initInfo.SrvDescriptorFreeFn = DescriptorFree;
    
    ImGui_ImplDX12_Init(&initInfo);

    engine->StartMainLoop();
    auto returnCode = engine->exitCode;

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    delete engine;

    //Device::ReportLiveObjects();

    return returnCode;
}