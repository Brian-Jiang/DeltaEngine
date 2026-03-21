#include "EditorMain.h"

#include "Editor/Assets/EditorAssetDatabase.h"
#include "Editor/EditorRenderManager.h"
#include "Editor/EditorSelectionState.h"
#include "Editor/EditorWindows/EditorWindow_AssetBrowser.h"
#include "Editor/EditorWindows/EditorWindow_ComponentsHierarchy.h"
#include "Editor/EditorWindows/EditorWindow_Details.h"
#include "Editor/EditorWindows/EditorWindow_Viewport.h"
#include "Editor/EditorWindows/EditorWindow_WorldOutliner.h"
#include "Editor/Style/EditorTheme.h"
#include "Runtime/Assets/AssetDatabaseLocator.h"
#include "Runtime/Assets/PA_DScene.h"
#include "Runtime/EngineMain.h"
#include "Runtime/Graphics/DirectX/Device.h"
#include "Runtime/Graphics/DirectX/SwapChain.h"
#include "Runtime/Graphics/DirectX/CommandQueue.h"
#include "Runtime/IO/IOManager.h"

#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>
#include <SDL3/SDL.h>

#include <chrono>
#include <cstdio>
#include <thread>

using namespace DeltaEngine;

EditorMain* DeltaEngine::g_editor = nullptr;

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

    m_assetDatabase = std::make_unique<EditorAssetDatabase>();
    AssetDatabaseLocator::Register(m_assetDatabase.get());
    m_assetDatabase->ScanAssetsFolder(IOManager::GetEngineImportedAssetsFolder());

    m_engine = std::make_unique<EngineMain>();
    m_engine->CreateWorld();
    m_engine->Initialize(m_renderManager->GetSceneRenderer());

    // CreateAssets();

    PA_DScene* sceneAsset = m_assetDatabase->LoadAsset<PA_DScene>(m_assetDatabase->FindAssetIdByPath(IOManager::GetEngineImportedAssetFullPath("DefaultScene")));
    if (!sceneAsset)
    {
        std::printf("Failed to load DefaultScene.\n");
        m_exitCode = 1;
        m_running = false;
        return;
    }

    m_engine->LoadScene(sceneAsset->GetAssetId());

    // m_engine->CreateGameObjects();

    // m_assetDatabase->SaveDirtyAssets();

    m_selectionState = std::make_unique<EditorSelectionState>();

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

        m_engine->PreTick();
        m_engine->Tick();
        m_renderManager->RenderFrame(m_engine.get());
    }

    m_engine->Cleanup();
    return m_engine->exitCode;
}

void EditorMain::RenderEditorWindows()
{
    for (const auto& window : m_editorWindows)
        window->Render();
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

//void EditorMain::CreateAssets()
//{
//    // Default scene
//    const std::filesystem::path scenePath = IOManager::GetEngineImportedAssetFullPath("DefaultScene");
//
//    PA_DScene* sceneAsset = PA_DScene::Create("DefaultScene");
//    m_assetDatabase->CreateAsset(scenePath, sceneAsset);
//    AssetId sceneId = sceneAsset->GetAssetId();
//
//    // Default shader
//    DShader* shader = CreateDObject<DShader>();
//    {
//        shader->Initialize(
//            L"Shaders.hlsl",
//            L"VSMain", L"PSMain",
//            L"vs_6_0", L"ps_6_0");
//
//        shader->SetInputLayout({
//            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
//        });
//
//        m_assetDatabase->CreateAsset(
//            IOManager::GetEngineImportedAssetFullPath("DefaultShader"),
//            PA_Shader::Create(shader));
//    }
//
//    // Star mesh and material
//    {
//        DMesh* mesh = CreateDObject<DMesh>();
//        mesh->Initialize(std::wstring(L"Star.obj"));
//        std::vector<DTexture*> textures = mesh->GetTextures();
//        std::vector<DMaterial*> materials;
//        for (int i = 0; i < mesh->GetSubMeshCount(); i++) {
//            DMaterial* material = CreateDObject<DMaterial>();
//            material->Initialize(shader);
//            if (i < textures.size()) {
//                m_assetDatabase->CreateAsset(
//                    IOManager::GetEngineImportedAssetFullPath("StarTexture_" + std::to_string(i)),
//                    PA_Texture::Create(textures[i]));
//                material->AddTexture(textures[i]);
//            }
//            materials.push_back(material);
//
//            m_assetDatabase->CreateAsset(
//                IOManager::GetEngineImportedAssetFullPath("StarMaterial_" + std::to_string(i)),
//                PA_Material::Create(material));
//        }
//
//        mesh->SetMaterials(materials);
//
//        m_assetDatabase->CreateAsset(
//            IOManager::GetEngineImportedAssetFullPath("StarMesh"),
//            PA_StaticMesh::Create(mesh));
//    }
//
//    // Home mesh and material
//    {
//        DMesh* mesh = CreateDObject<DMesh>();
//        mesh->Initialize(std::wstring(L"home/source/home.fbx"));
//        std::vector<DTexture*> textures = mesh->GetTextures();
//        std::vector<DMaterial*> materials;
//        for (int i = 0; i < mesh->GetSubMeshCount(); i++) {
//            DMaterial* material = CreateDObject<DMaterial>();
//            material->Initialize(shader);
//            if (i < textures.size()) {
//                m_assetDatabase->CreateAsset(
//                    IOManager::GetEngineImportedAssetFullPath("HomeTexture_" + std::to_string(i)),
//                    PA_Texture::Create(textures[i]));
//                material->AddTexture(textures[i]);
//            }
//            materials.push_back(material);
//
//            m_assetDatabase->CreateAsset(
//                IOManager::GetEngineImportedAssetFullPath("HomeMaterial_" + std::to_string(i)),
//                PA_Material::Create(material));
//        }
//
//        mesh->SetMaterials(materials);
//
//        m_assetDatabase->CreateAsset(
//            IOManager::GetEngineImportedAssetFullPath("HomeMesh"),
//            PA_StaticMesh::Create(mesh));
//    }
//}

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
            if (key == SDLK_F11)
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

    m_selectionState.reset();
    m_editorWindows.clear();
    m_editorTheme.reset();
    m_assetDatabase.reset();
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
