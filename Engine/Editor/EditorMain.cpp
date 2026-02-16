#include "EditorMain.h"

#include <SDL3/SDL.h>
#include <stdio.h>

#include "Runtime/Graphics/DXRenderManager.h"
#include "Runtime/Graphics/DirectX/SwapChain.h"
#include "Editor/EditorWindows/EditorWindow_WorldOutliner.h"
#include "Editor/EditorWindows/EditorWindow_Viewport.h"
#include "Editor/EditorWindows/EditorWindow_ComponentsHierarchy.h"
#include "Editor/EditorWindows/EditorWindow_Details.h"

#include "imgui.h"
#include "backends/imgui_impl_sdl3.h"
#include "backends/imgui_impl_dx12.h"

using namespace DeltaEngine;

EditorMain* DeltaEngine::g_editor = nullptr;


static void ImGuiDescriptorAllocate(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu)
{
    auto* allocator = static_cast<ImGuiSrvDescriptorAllocator*>(info->UserData);
    allocator->Alloc(out_cpu, out_gpu);
}

static void ImGuiDescriptorFree(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
{
    auto* allocator = static_cast<ImGuiSrvDescriptorAllocator*>(info->UserData);
    allocator->Free(cpu, gpu);
}

EditorMain::EditorMain()
{
    g_editor = this;
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        m_exitCode = 1;
        m_running = false;
        return;
    }

    m_window = std::shared_ptr<SDL_Window>(SDL_CreateWindow("Delta Editor", DEFAULT_WIDTH, DEFAULT_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY), SDL_DestroyWindow);
    if (!m_window)
    {
        printf("Failed to create window: %s\n", SDL_GetError());
        m_exitCode = 1;
        m_running = false;
        return;
    }

    auto hwnd = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(m_window.get()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    m_renderManager = std::make_unique<EditorRenderManager>(hwnd, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    m_renderManager->ToggleVSync(false);

    m_engine = std::make_unique<EngineMain>();
    m_engine->Initialize(m_renderManager->GetSceneRenderer(), m_window);

    m_selectionState = std::make_unique<EditorSelectionState>();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigDpiScaleFonts = true; // (Docking branch only) Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
    io.ConfigDpiScaleViewports = true; // (Docking branch only) Scale Dear ImGui and Platform Windows when Monitor DPI changes.
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplSDL3_InitForD3D(m_window.get());

    // todo get from sdl3 window
    float main_scale = 2.0f;
    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    //style.FontSizeBase = 20.0f;
    style.ScaleAllSizes(main_scale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    ImGui_ImplDX12_InitInfo imguiInit = {};
    imguiInit.Device = m_renderManager->GetDevice()->GetD3D12Device().Get();
    imguiInit.CommandQueue = m_renderManager->GetDevice()->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT).GetD3D12CommandQueue().Get();
    imguiInit.NumFramesInFlight = SwapChain::BufferCount;
    imguiInit.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    imguiInit.DSVFormat = DXGI_FORMAT_UNKNOWN;
    imguiInit.SrvDescriptorHeap = m_renderManager->GetImGuiSrvAllocator()->GetHeap();
    imguiInit.UserData = m_renderManager->GetImGuiSrvAllocator();
    imguiInit.SrvDescriptorAllocFn = ImGuiDescriptorAllocate;
    imguiInit.SrvDescriptorFreeFn = ImGuiDescriptorFree;
    ImGui_ImplDX12_Init(&imguiInit);
}

EditorMain::~EditorMain()
{
    g_editor = nullptr;
    Shutdown();
}

int EditorMain::Run()
{
    using Clock = std::chrono::high_resolution_clock;
    using namespace std::chrono;

    auto targetFrameTime = duration<double>(1.0 / 60.0); // 60 FPS cap
    auto lastFrame = Clock::now();

    OpenEditorWindow<EditorWindow_WorldOutliner>();
    OpenEditorWindow<EditorWindow_Viewport>();
    OpenEditorWindow<EditorWindow_ComponentsHierarchy>();
    OpenEditorWindow<EditorWindow_Details>();

    // todo check if needs redraw
    while (m_running)
    {
        auto now = Clock::now();
        auto elapsed = now - lastFrame;
        if (elapsed < targetFrameTime) {
            // Sleep for most of the remaining time (saves CPU)
            auto remaining = targetFrameTime - elapsed;
            if (remaining > 1ms) {
                std::this_thread::sleep_for(remaining - 1ms); // leave 1ms margin
            }
            continue;
        }

        lastFrame = now;

        ProcessEvents();
        if (!m_running)
            break;

        m_engine->PreTick();
        m_engine->Tick();
        m_renderManager->RenderFrame(m_engine.get());
    }

    m_engine->Cleanup();

    return m_engine->exitCode;
}

void DeltaEngine::EditorMain::RenderEditorWindows()
{
    for (const auto& window : m_editorWindows)
    {
        window->Render();
    }
}

void EditorMain::ProcessEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                m_running = false;
                m_engine->gameState = GameState::EXIT;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                m_renderManager->Resize(event.window.data1, event.window.data2);
                m_engine->OnWindowResized(event.window.data1, event.window.data2);
                break;
            case SDL_EVENT_KEY_DOWN:
            {
                SDL_Keycode key = event.key.key;
                if (key == SDLK_F11)
                    m_renderManager->SetFullscreen(!m_renderManager->IsFullscreen());
                //else if (key == SDLK_V)
                //    m_renderManager->ToggleVSync(!m_renderManager->IsVSync());
                else
                    m_engine->ProcessEvent(event);
                break;
            }
            default:
                m_engine->ProcessEvent(event);
                break;
        }
    }
}

void EditorMain::Shutdown()
{
    if (m_engine && m_renderManager)
        m_renderManager->OnDestroy();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    m_renderManager.reset();
    m_engine.reset();

    if (m_window)
    {
        SDL_DestroyWindow(m_window.get());
        m_window.reset();
    }

    SDL_Quit();

    Device::ReportLiveObjects();
}
