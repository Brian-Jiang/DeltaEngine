#include "EditorMain.h"

#include "Editor/EditorCore.h"
#include "Editor/Mcp/McpQueryRouter.h"
#include "Editor/Mcp/McpRegistry.h"
#include "Editor/McpSocketServer.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Editor/Commands/EditorAuxiliarySceneCommands.h"
#include "Editor/Commands/EditorCommandContext.h"
#include "Editor/Commands/EditorCommandManager.h"
#include "Editor/EditorRenderManager.h"
#include "Editor/EditorWindows/EditorWindow_AssetBrowser.h"
#include "Editor/EditorWindows/EditorWindow_ComponentsHierarchy.h"
#include "Editor/EditorWindows/EditorWindow_Details.h"
#include "Editor/EditorWindows/EditorWindow_Viewport.h"
#include "Editor/EditorWindows/EditorWindow_WorldOutliner.h"
#include "Editor/Style/EditorTheme.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/SwapChain.h"
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

using namespace DeltaEngine;

EditorMain* DeltaEngine::g_editor = nullptr;
static std::unique_ptr<McpSocketServer> g_mcpServer;

namespace
{
void ImGuiDescriptorAllocate(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu)
{
    auto* allocator = static_cast<ImGuiSrvDescriptorAllocator*>(info->UserData);
    allocator->Alloc(outCpu, outGpu);
}

void ImGuiDescriptorFree(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
{
    auto* allocator = static_cast<ImGuiSrvDescriptorAllocator*>(info->UserData);
    allocator->Free(cpu, gpu);
}
}

EditorMain::EditorMain()
{
    g_editor = this;
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        m_exitCode = 1;
        m_running = false;
        return;
    }
    m_sdlInitialized = true;

    m_window = std::shared_ptr<SDL_Window>(
        SDL_CreateWindow("Delta Editor", DEFAULT_WIDTH, DEFAULT_HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_MAXIMIZED),
        SDL_DestroyWindow);
    if (!m_window)
    {
        std::printf("Failed to create window: %s\n", SDL_GetError());
        m_exitCode = 1;
        m_running = false;
        return;
    }

    auto hwnd = static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(m_window.get()), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
    if (!hwnd)
    {
        std::printf("Failed to acquire a Win32 window handle.\n");
        m_exitCode = 1;
        m_running = false;
        return;
    }

    m_renderManager = std::make_unique<EditorRenderManager>(hwnd, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    m_renderManager->ToggleVSync(false);

    m_engine = std::make_unique<EngineMain>();
    m_engine->CreateWorld();
    m_engine->Initialize(m_renderManager->GetSceneRenderer());

    m_editorCore = std::make_unique<EditorCore>();
    m_editorCore->Initialize(*m_engine);

    McpRegistry::Get().InitializeAll(*m_editorCore);

    auto router = std::make_shared<McpQueryRouter>(*m_editorCore);
    g_mcpServer = std::make_unique<McpSocketServer>(
        [](const std::string& json) {
            g_editorCore->EnqueueSerializedCommand(json);
        },
        [router](const std::string& json) -> std::string {
            return router->Route(json);
        }
    );
    g_mcpServer->Start();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_imguiContextCreated = true;

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigDpiScaleViewports = true;

    m_editorTheme = std::make_unique<EditorTheme>();
    m_editorTheme->ApplyTheme();

    if (!ImGui_ImplSDL3_InitForD3D(m_window.get()))
    {
        std::printf("Failed to initialize the ImGui SDL3 backend.\n");
        m_exitCode = 1;
        m_running = false;
        return;
    }
    m_imguiSdlInitialized = true;

    constexpr float kMainScale = 1.5f;
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(kMainScale);
    style.FontScaleDpi = kMainScale;

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
    if (!ImGui_ImplDX12_Init(&imguiInit))
    {
        std::printf("Failed to initialize the ImGui DX12 backend.\n");
        m_exitCode = 1;
        m_running = false;
        return;
    }
    m_imguiDx12Initialized = true;
}

EditorMain::~EditorMain()
{
    g_editor = nullptr;
    Shutdown();
}

ImTextureID EditorMain::GetSceneTextureId() const
{
    return m_renderManager ? m_renderManager->GetSceneTextureId() : ImTextureID {};
}

int EditorMain::Run()
{
    if (!m_running || !m_engine || !m_renderManager)
        return m_exitCode;

    using Clock = std::chrono::high_resolution_clock;
    using namespace std::chrono;

    const auto targetFrameTime = duration<double>(1.0 / 60.0);
    auto lastFrame = Clock::now();

    OpenEditorWindow<EditorWindow_WorldOutliner>();
    OpenEditorWindow<EditorWindow_Viewport>();
    OpenEditorWindow<EditorWindow_ComponentsHierarchy>();
    OpenEditorWindow<EditorWindow_Details>();
    OpenEditorWindow<EditorWindow_AssetBrowser>();

    // todo check if needs redraw
    while (m_running)
    {
        const auto now = Clock::now();
        const auto elapsed = now - lastFrame;
        if (elapsed < targetFrameTime)
        {
            const auto remaining = targetFrameTime - elapsed;
            if (remaining > 1ms)
                std::this_thread::sleep_for(remaining - 1ms);
            continue;
        }

        lastFrame = now;

        ProcessEvents();
        if (!m_running)
            break;

        std::vector<std::string> responses;
        m_editorCore->DrainCommandQueue(responses);
        if (g_mcpServer) {
            for (const auto& r : responses)
                g_mcpServer->SendResponse(r);
        }
        m_engine->PreTick();
        m_engine->Tick();
        m_renderManager->RenderFrame(m_engine.get());
    }

    m_engine->Cleanup();
    return m_engine->exitCode;
}

void EditorMain::RenderEditorWindows()
{
    // Remove non-singleton windows that the user has closed.
    m_editorWindows.erase(
        std::remove_if(m_editorWindows.begin(), m_editorWindows.end(),
            [](const auto& info) { return info.m_window->ShouldDestroyOnClose() && !info.m_open; }),
        m_editorWindows.end());

    // Render remaining windows; skip singleton windows that are currently hidden.
    for (auto& info : m_editorWindows)
    {
        if (!info.m_open)
            continue;
        info.m_window->Render(info.m_open);
    }
}

std::vector<EditorWindowInfo>& EditorMain::GetEditorWindowInfos()
{
    return m_editorWindows;
}

void EditorMain::OpenViewportWindow()
{
    OpenEditorWindow<EditorWindow_Viewport>();  // Non-singleton: always creates a new instance.
}

void EditorMain::SetSceneRenderSize(UINT width, UINT height)
{
    if (!m_renderManager || !m_engine)
        return;

    m_renderManager->SetSceneRenderSize(width, height);
    m_engine->OnWindowResized(width, height);
}

void EditorMain::GetSceneRenderSize(UINT& width, UINT& height) const
{
    if (!m_renderManager)
    {
        width = 0;
        height = 0;
        return;
    }

    m_renderManager->GetSceneRenderSize(width, height);
}

void EditorMain::SetPreviewCameraOverride(const CameraCB& cb)
{
    if (m_renderManager)
        m_renderManager->SetPreviewCameraOverride(cb);
}

void EditorMain::ClearPreviewCameraOverride()
{
    if (m_renderManager)
        m_renderManager->ClearPreviewCameraOverride();
}

void EditorMain::ProcessEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (m_imguiSdlInitialized)
            ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type)
        {
        case SDL_EVENT_QUIT:
            m_running = false;
            if (m_engine)
                m_engine->gameState = GameState::EXIT;
            break;
        case SDL_EVENT_WINDOW_RESIZED:
            if (m_renderManager)
                m_renderManager->Resize(event.window.data1, event.window.data2);
            break;
        case SDL_EVENT_KEY_DOWN:
        {
            const SDL_Keycode key = event.key.key;
            if ((event.key.mod & SDL_KMOD_CTRL) && key == SDLK_S)
            {
                EditorCommandContext ctx{ *m_editorCore };
                m_editorCore->GetCommandManager().ExecuteAuxiliary(
                    std::make_unique<EditorAuxiliaryCommand_SaveScene>(), ctx);
            }
            else if ((event.key.mod & SDL_KMOD_CTRL) && key == SDLK_Z)
            {
                EditorCommandContext ctx{ *m_editorCore };
                if (event.key.mod & SDL_KMOD_SHIFT)
                    m_editorCore->GetCommandManager().Redo(ctx);
                else
                    m_editorCore->GetCommandManager().Undo(ctx);
            }
            else if ((event.key.mod & SDL_KMOD_CTRL) && key == SDLK_Y)
            {
                EditorCommandContext ctx{ *m_editorCore };
                m_editorCore->GetCommandManager().Redo(ctx);
            }
            else if (key == SDLK_F11)
            {
                if (m_renderManager)
                    m_renderManager->SetFullscreen(!m_renderManager->IsFullscreen());
            }
            else if (m_engine)
            {
                m_engine->ProcessEvent(event);
            }
            break;
        }
        default:
            if (m_engine)
                m_engine->ProcessEvent(event);
            break;
        }
    }
}

void EditorMain::Shutdown()
{
    if (m_renderManager)
        m_renderManager->OnDestroy();

    if (m_imguiDx12Initialized)
    {
        ImGui_ImplDX12_Shutdown();
        m_imguiDx12Initialized = false;
    }

    if (m_imguiSdlInitialized)
    {
        ImGui_ImplSDL3_Shutdown();
        m_imguiSdlInitialized = false;
    }

    if (m_imguiContextCreated)
    {
        ImGui::DestroyContext();
        m_imguiContextCreated = false;
    }

    m_editorWindows.clear();
    m_editorTheme.reset();
    g_mcpServer.reset();
    m_editorCore.reset();
    m_renderManager.reset();
    m_engine.reset();
    m_window.reset();

    if (m_sdlInitialized)
    {
        SDL_Quit();
        m_sdlInitialized = false;
    }

    Device::ReportLiveObjects();
}
