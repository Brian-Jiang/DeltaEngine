# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DeltaEngine is a DirectX 12 game engine with an integrated editor, written in C++23. The editor uses ImGui for UI and SDL3 for window management.

## Build System

**CMake + Ninja**, targeting Windows x64. The only configured preset is `x64-debug`.

```bash
# Configure (from repo root)
cmake --preset x64-debug

# Build
cmake --build Build/x64-Debug

# Or open in Visual Studio via CMake integration (configured in .vs/)
```

Build output goes to `Build/x64-Debug/bin/` (executables) and `Build/x64-Debug/lib/` (libraries). The `CopyDxcBin` custom target copies DXC compiler binaries to the output directory automatically.

**Compiler requirements:** MSVC with C++23, `/permissive-` (strict), `/MP` (parallel compilation). No test suite exists — the editor is the primary verification tool.

## Architecture

The project is split into two CMake targets:

- **DeltaEngine** (`Engine/Runtime/`) — static library: core, graphics, assets, importers
- **DeltaEditor** (`Engine/Editor/`) — executable: editor app, ImGui windows, selection state

### Frame Loop

```
EditorMain::Run()
  ├── EngineMain::PreTick()          // update Time
  ├── EngineMain::Tick()             // game logic
  ├── EngineMain::ProcessEvent()     // SDL input
  ├── EditorRenderManager::BeginFrame()
  │     └── DXRenderManager::RecordSceneDraws(context)
  │           └── DWorld → GameObjects → Renderers → GatherDrawCalls()
  ├── EditorRenderManager::RenderImGui()  // all editor windows
  └── EditorRenderManager::Present()
```

The scene renders to an **offscreen render target** inside `DXRenderManager`; `EditorRenderManager` composites it into the ImGui viewport and presents to the swap chain.

### Scene / Component Hierarchy

```
DWorld
└── GameObject[]
      ├── SceneComponent (root transform, parent-child tree)
      │     ├── MeshRenderer  → DMesh + DMaterial[]
      │     ├── Camera
      │     ├── DirectionalLight / PointLight / SpotLight
      │     └── (nested SceneComponents)
      └── DComponent  (non-spatial components)
```

- `DObject` is the base for everything. `GameObject`, `DComponent`, `SceneComponent` all inherit from it.
- Use `enable_shared_from_this` — always hold engine objects in `shared_ptr`.
- C++20 concepts (`IsDComponent`, `IsSceneComponent`, `IsEditorWindow`) are used for type-safe template APIs.

### Graphics Layer (`Engine/Runtime/Graphics/DirectX/`)

All DirectX 12 objects are wrapped:

| Wrapper | Wraps |
|---|---|
| `Device` | `ID3D12Device`; factory for buffers, textures, PSOs, descriptor heaps |
| `CommandList` | Records barriers, draws, copies; owns `ResourceStateTracker` |
| `CommandQueue` | Executes command lists, manages fences |
| `SwapChain` | DXGI swap chain + present |
| `PipelineStateObject` | PSO caching |
| `DescriptorAllocator` | CPU descriptor heap free-list |
| `DynamicDescriptorHeap` | GPU-visible descriptor heap per frame |

`DXGraphicsContext` is passed through the render call stack carrying the device, command list, camera, and light data for the current frame.

### Rendering Components (`Engine/Runtime/Graphics/Renderer/`)

`Renderer` (base) → `MeshRenderer`, `SpriteRenderer`. Renderers are `SceneComponent` subclasses and implement `GatherDrawCalls(DXGraphicsContext&)`.

**RenderProxy** objects (`MeshRenderProxy`, `CameraRenderProxy`, etc.) decouple scene data from the GPU submission.

### Asset Pipeline

- **Models:** `ModelImporter` via Assimp → `DMesh` (submeshes with vertices/indices) + `DMaterial`
- **Textures:** `TextureImporter` via DirectXTex / stb_image → `DTexture` → `DirectX12Texture`
- **Shaders:** `DShader` compiles HLSL at runtime using DXC (`dxcompiler.dll`). Sources in `Engine/Runtime/Shaders/`.

### Editor Windows (`Engine/Editor/EditorWindows/`)

All editor windows implement `EditorWindow` interface. Current windows:
- `EditorWindow_Viewport` — displays scene texture, fly camera (WASD + mouse)
- `EditorWindow_WorldOutliner` — scene hierarchy tree
- `EditorWindow_ComponentsHierarchy` — components of selected GameObject
- `EditorWindow_Details` — property inspector

`EditorSelectionState` (singleton-like, owned by `EditorMain`) is the shared selection model passed to all editor windows.

## Key Conventions

- **Headers only for declarations/implementations split:** most files use `.h` + `.cpp` pairs under the same directory.
- **Smart pointers everywhere:** `shared_ptr`/`weak_ptr` for scene objects, `unique_ptr` for owned subsystems.
- **No raw `new`/`delete`** for engine objects.
- **HLSL shaders** live alongside engine source in `Engine/Runtime/Shaders/` and are compiled at runtime (not offline). The `StandardObject.hlsl` / `StandardLighting.hlsl` / `StandardConstantStructs.hlsl` trio forms the standard material shader.
- **`DXGraphicsContext`** is the primary way to pass rendering state down the call stack — do not add global graphics state.
