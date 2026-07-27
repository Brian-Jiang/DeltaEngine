# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DeltaEngine is a DirectX 12 game engine with an integrated editor, written in C++23. The editor uses ImGui for UI and SDL3 for window management. The engine has a full static reflection system driven by a Python-based code generator (`DeltaHeaderTool`).

## Build System

**CMake + Ninja**, targeting Windows x64. The only configured preset is `x64-debug`.

**Always drive builds, runs, and tests through `Tools/Scripts/DeltaCmd.bat`** — it sets up `DELTA_PROJECT_ROOT`, the bundled Python, and the VS dev environment. Do not call `cmake` / `pytest` directly from tool or agent invocations.

```bash
Tools\Scripts\DeltaCmd.bat --automatic list                    # discover presets, targets, and examples
Tools\Scripts\DeltaCmd.bat --automatic configure             # cmake --preset x64-debug
Tools\Scripts\DeltaCmd.bat --automatic build editor          # incremental build (DeltaEditorLaunch)
Tools\Scripts\DeltaCmd.bat --automatic build engine-tests    # build DeltaEngineTests
Tools\Scripts\DeltaCmd.bat --automatic build editor-tests    # build DeltaEditorTests
Tools\Scripts\DeltaCmd.bat --automatic run editor            # launch DeltaEditorLaunch.exe
Tools\Scripts\DeltaCmd.bat --automatic test engine           # run DeltaEngineTests (GTest)
Tools\Scripts\DeltaCmd.bat --automatic test editor           # run DeltaEditorTests (GTest)
Tools\Scripts\DeltaCmd.bat --automatic test header-tool      # pytest for DeltaHeaderTool
Tools\Scripts\DeltaCmd.bat --automatic test delta-cmd        # pytest for DeltaCmd
Tools\Scripts\DeltaCmd.bat --automatic header generate       # incremental reflection codegen
Tools\Scripts\DeltaCmd.bat --automatic header force          # full reflection codegen
```

Legacy `.bat` aliases (e.g. `build-x64-debug.bat`, `run-x64-debug-engine-tests.bat`) forward to the same DeltaCmd subcommands and remain valid.

| Legacy `.bat` | DeltaCmd equivalent |
|---|---|
| `configure-x64-debug.bat` | `configure` |
| `rebuild-x64-debug.bat` | `configure` then `build editor` |
| `build-x64-debug.bat` / `build.bat` | `build editor` |
| `build-x64-debug-engine-tests.bat` | `build engine-tests` |
| `build-x64-debug-editor-tests.bat` | `build editor-tests` |
| `run-x64-debug.bat` | `run editor` |
| `run-x64-debug-engine-tests.bat` | `test engine` |
| `run-x64-debug-editor-tests.bat` | `test editor` |
| `delta_header_generate.bat` | `header generate` |
| `delta_header_force_generate.bat` | `header force` |
| `test-delta-header-tool.bat` | `test header-tool` |
| `test-delta-cmd.bat` | `test delta-cmd` |

### `--automatic` flag (MANDATORY for tool/agent invocations)

Every DeltaCmd invocation (and legacy `.bat` forwarders) pauses at the end by default so a human double-clicking can read the output. **When invoked by Claude Code, an agent, CI, or any other non-interactive context, always pass `--automatic`** so the CLI skips the trailing pause and returns the real exit code. Omitting it will hang the tool call forever waiting on a keypress.

```bash
Tools\Scripts\DeltaCmd.bat --automatic build editor
Tools\Scripts\DeltaCmd.bat --automatic test engine
Tools\Scripts\DeltaCmd.bat --automatic test engine -- --gtest_filter=Foo*   # GTest filter example
Tools\Scripts\DeltaCmd.bat --automatic test header-tool -k test_parser    # pytest filter example
Tools\Scripts\DeltaCmd.bat --automatic test delta-cmd
```

**Verifying code modifications:** After any change to engine or editor code, run the relevant test suite to confirm nothing regressed:
```bash
Tools\Scripts\DeltaCmd.bat --automatic build engine-tests && Tools\Scripts\DeltaCmd.bat --automatic test engine
Tools\Scripts\DeltaCmd.bat --automatic build editor-tests && Tools\Scripts\DeltaCmd.bat --automatic test editor
```

Build output goes to `Build/x64-Debug/bin/` (executables) and `Build/x64-Debug/lib/` (libraries). The `CopyDxcBin` custom target copies DXC compiler binaries to the output directory automatically.

**Compiler requirements:** MSVC with C++23, `/permissive-` (strict), `/MP` (parallel compilation). A GTest-based test suite lives in `Engine/Tests/` (see [Tests](#tests) below).

### DeltaHeaderTool Build Integration

`DeltaHeaderTool` is a code generator that runs automatically as part of every build:

1. **Configure time** — if the manifest is missing (first configure or clean), the tool runs once to produce `Intermediate/DeltaHeaderTool/generated_sources.cmake`.
2. **Build time** — custom target `DeltaHeaderToolRun` always executes before `DeltaEngine`, running the tool with timestamp-based incremental checks (only files with updated sources are regenerated).
3. The manifest lists all generated `.cpp` files; CMake automatically reconfigures if it changes.

```bash
# Force full regeneration (VS: build the DeltaHeaderTool target)
cmake --build Build/x64-Debug --target DeltaHeaderTool

# Or via DeltaCmd
Tools\Scripts\DeltaCmd.bat --automatic header force
```

### CI/CD (`.github/workflows/ci.yml`)

GitHub Actions on **self-hosted Windows runners**. Triggers: PRs targeting `main` or `dev/**`; pushes to `main`. Pipeline: DeltaCmd self-tests → DeltaMCP tests → configure/build editor → build engine tests → run engine tests → build editor tests → run editor tests. Build tree (`Build/x64-Debug`) and reflection headers (`Intermediate/DeltaHeaderTool`) are cached per branch. Cache is only saved on pushes to `main`.

## Architecture

The project is split into two CMake targets:

- **DeltaEngine** (`Engine/Runtime/`) — shared library: core, graphics, assets, reflection, serialization
- **DeltaEditor** (`Engine/Editor/`) — executable: editor app, fixed chrome panels, docked editor windows, selection state

### Frame Loop

```
EditorMain::Run()
  ├── EngineMain::PreTick()          // update Time
  ├── EngineMain::Tick()             // game logic
  ├── EngineMain::ProcessEvent()     // SDL input
  └── EditorRenderManager::RenderFrame()
        ├── draw fixed editor panels + dock host
        ├── DXRenderManager::PrepareFrame()
        ├── EngineMain::RecordSceneDraws(context)
        │     └── DWorld → GameObjects → Renderers → GatherDrawCalls()
        ├── DXRenderManager::RenderFrame()
        ├── render docked editor windows + ImGui
        └── SwapChain::Present()
```

The scene renders to an **offscreen render target** inside `DXRenderManager`; `EditorRenderManager` composites it into the ImGui viewport and presents to the swap chain.

### Scene / Component Hierarchy

```
DWorld
└── DScene (active scene — serializable, owns GameObjects + optional Skybox)
└── GameObject[]
      ├── SceneComponent (root transform, parent-child tree)
      │     ├── MeshRenderer  → DMesh + DMaterial[]
      │     ├── Camera
      │     ├── DirectionalLight / PointLight / SpotLight
      │     └── (nested SceneComponents)
      └── DComponent  (non-spatial components)
```

`DScene` is the serialized scene representation (stored in `PA_DScene` asset). It holds `m_gameObjects`, `m_components`, and an optional `Skybox* m_skybox`. `DWorld` is the runtime container; it syncs the skybox from the active `DScene` and draws it last in `GatherDrawCalls`.

- `DObject` is the base for everything. `GameObject`, `DComponent`, `SceneComponent` all inherit from it.
- `DObject`-derived relationships in reflected/runtime gameplay data are represented with raw pointers (`T*`).
- Asset ownership is centralized in `DPrimaryAsset`, which stores loaded objects as `std::shared_ptr<DObject>` and returns raw pointers for lookup/use.
- C++20 concepts (`IsDComponent`, `IsSceneComponent`, `IsEditorWindow`) are used for type-safe template APIs.

### GC System (`Engine/Runtime/Core/GC/`)

A blocking, two-phase garbage collector. All `DObject` instances are registered in `DObjectRegistry` at construction.

| Type | Role |
|---|---|
| `DObjectHandle` | Versioned slot reference (`m_slotIndex` + `m_version`); stale handles resolve to `nullptr` |
| `DObjectRegistry` | Singleton slot table; `RegisterObject`, `FreeSlot`, `Resolve`, `AddRoot`/`RemoveRoot` |
| `GCManager` | `Mark()` colors reachable objects; `Tick()` advances Idle → Marking → Sweeping; `CollectGarbage()` runs a full cycle synchronously |
| `StrongDObjectPtr<T>` | RAII root holder; keeps target alive by calling `AddRoot`; use instead of raw `T*` for long-lived cross-system references |
| `WeakDObjectPtr<T>` | Non-owning; auto-nulls once target is freed (version mismatch); can be constructed from a `StrongDObjectPtr` or raw pointer |

**DObject lifecycle hooks** (override in subclasses for GPU resource teardown):
- `BeginDestroy()` — called when the object enters `PendingKill`; start releasing GPU handles
- `IsReadyForFinishDestroy()` — GC polls each frame; return `true` once async teardown is complete
- `FinishDestroy()` — called once ready; object is freed immediately after

`GCManager::CollectAllForShutdown()` destroys every live object synchronously at engine shutdown — call it before tearing down the device.

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

**PBR / IBL support (`Engine/Runtime/Graphics/IBL/`, `Engine/Runtime/Graphics/`):**

| Class / File | Role |
|---|---|
| `IBLBaker` | Bakes irradiance cube, specular cube, and BRDF LUT from a source HDR cubemap; owned by `DXRenderManager` |
| `IBLBaker::IBLResources` | Holds the three baked `DirectX12Texture` objects + their CPU SRV handles |
| `DefaultTextures` | Static singleton providing engine-wide fallback textures: 1×1 white 2D, 1×1×6 black cubemap, 1×1 black RG (BRDF LUT fallback); call `Initialize(device, commandList)` once |
| `MaterialConstants.h` | `MaterialFlags` enum (HasAlbedoMap … DoubleSided), `MaterialTextureSlot` enum, `MaterialCB` (256-byte aligned constant buffer: baseColor, metallic, roughness, emissiveColor, emissiveIntensity, alphaCutoff, flags) |

`DXRenderManager` calls `UpdateIBL(cubemap)` whenever the active skybox cubemap changes and binds the resulting `IBLResources` to the `IBLTextures` root parameter slot each frame.

### RenderGraph (`Engine/Runtime/Graphics/RenderGraph/`)

Frame rendering is structured as a `RenderGraph` of `RenderGraphPass` instances. Barriers and clears are inserted automatically at `Compile()` time based on declared access.

| Class | Role |
|---|---|
| `RenderGraph` | Owns passes and imported/transient textures; `AddPass`, `ImportTexture`, `CreateTexture`, `Compile`, `Execute`, `Reset` |
| `RenderGraphPass` | Abstract base; subclasses implement `GetName()`, `Setup(builder)`, `Execute(context)` |
| `RenderGraphBuilder` | Used inside `Setup()` to declare `Read(handle, state)` / `Write(handle, state)` access |
| `RenderGraphTextureHandle` | Opaque index into the graph's texture list; obtained from `ImportTexture` / `CreateTexture` |
| `TransientTexturePool` | Frame-scoped texture recycler; `Acquire(desc)`, `BeginFrame(fenceCompleted)`, `RetireFrame(fenceSubmitted)`; entries unused for `kMaxIdleFrames = 3` cycles are destroyed |

**Built-in passes:**

| Pass | Role |
|---|---|
| `GBufferRenderGraphPass` | Writes albedo / normal / material / emissive / depth G-Buffer targets |
| `DeferredLightingGraphPass` | Reads G-Buffer SRVs, outputs scene color |
| `SceneRenderGraphPass` | Forward scene draw (non-deferred path) |
| `ShadowRenderGraphPass` | Shadow map generation |
| `SceneShadowReadGraphPass` | Transitions shadow map for read |
| `SkyboxRenderGraphPass` | Skybox draw after scene |
| `PostProcessRenderGraphPass` | Runs `PostProcessStack` passes |
| `PostProcessFinalizeGraphPass` | Final blit to swap-chain target |
| `MsaaResolveGraphPass` | MSAA resolve step |

**Adding a new pass:** subclass `RenderGraphPass`, declare reads/writes in `Setup()`, record commands in `Execute()`. Do not manually insert resource barriers — the graph handles them.

**Deferred rendering shaders:** `GBuffer.slang` (G-Buffer fill) and `DeferredLighting.slang` (lighting resolve) are the deferred shader pair, alongside the existing PBR and Standard sets.

### Rendering Components (`Engine/Runtime/Graphics/Renderer/`)

`Renderer` (base) → `MeshRenderer`, `SpriteRenderer`. Renderers are `SceneComponent` subclasses and implement `GatherDrawCalls(DXGraphicsContext&)`.

**RenderProxy** objects (`MeshRenderProxy`, `CameraRenderProxy`, `SkyboxRenderProxy`, etc.) decouple scene data from the GPU submission.

**Vertex layout** (`Engine/Runtime/Graphics/Structures/Vertex.h`): `position` (XMFLOAT3), `normal` (XMFLOAT3), `tangent` (XMFLOAT3), `uv` (XMFLOAT2).

**Root parameter slots** (`RootParameterType.h`): `PerObject`, `PerFrame`, `Textures`, `PointLights`, `DirectionalLights`, `MaterialCB` (per-material constant buffer), `IBLTextures` (t0–t2 space2: irradiance cube, specular cube, BRDF LUT).

### Skybox (`Engine/Runtime/Core/Skybox.h`)

`Skybox` is a reflected `DObject` with two DPROPERTYs: `DTexture* m_cubemapTexture` (must be a DDS cubemap) and `DMaterial* m_material` (holds the compiled skybox shader). Call `Initialize(context)` once after both are set — it compiles the shader, uploads the cubemap to the GPU, and builds the PSO. `GetRenderProxy()` returns the `SkyboxRenderProxy` (owns the GPU cubemap). `GatherDrawCalls(context)` records the draw. The scene-level asset wrapper is `PA_Skybox`. `Skybox.slang` is the skybox shader.

### Post-Processing (`Engine/Runtime/Graphics/PostProcess/`)

A reflected, extensible post-process pipeline:

| Class | Role |
|---|---|
| `PostProcessPass` | Abstract DCLASS base; `m_passName`, `m_enabled`, `m_shader`; subclasses implement `Initialize` + `Execute` |
| `PostProcessStack` | DCLASS owning a `std::vector<PostProcessPass*> m_passes` |
| `PA_PostProcessStack` | Primary asset wrapper for a `PostProcessStack` |
| `PassthroughPass` | Pass-through blit |
| `TonemapPass` | HDR → LDR tone-mapping |

New post-process passes subclass `PostProcessPass`, annotate with `DCLASS()`, and call `ResolveShader` (or set `m_shader`) in `Initialize`.

### Asset Pipeline

- **Models:** `DMesh::Initialize(...)` currently imports directly via Assimp and builds submesh/material/texture data.
- **Textures:** `DTexture` initialization currently goes through `TextureImporter`; the standalone importer layer exists in `Engine/Runtime/Importers/` but is not an active high-level pipeline right now.
- **Shaders:** `DShader` compiles shaders at runtime. Sources in `Engine/EngineSourceAssets/Shaders/`. Shaders are now written in **Slang** (`.slang` files). `CompileSlangStage(engineRelativePath, entryPoint, targetProfile)` compiles a single stage via the Slang C++ API; `CompileHLSLStage` remains for legacy use. Both functions live in `Engine/Runtime/Graphics/ShaderCompile.h`.
  - **Standard shader quartet:** `StandardObject.slang` / `StandardLighting.slang` / `StandardConstantStructs.slang` / `StandardInputs.slang`
  - **PBR shader set:** `PBRObject.slang` / `PBRLighting.slang` / `PBRInputs.slang` — physically-based rendering with IBL support
  - **IBL bake shaders:** `IBL_BrdfLut.slang`, `IBL_IrradianceConvolve.slang`, `IBL_SpecularPrefilter.slang`, `IBL_Math.slang`
  - **Deferred shaders:** `GBuffer.slang` (G-Buffer fill), `DeferredLighting.slang` (deferred lighting resolve)

### Editor UI (`Engine/Editor/`)

The editor now has a fixed-position chrome layer plus docked content windows:

- **Fixed panels** (`Engine/Editor/Panels/`): `AppHeader`, `MainToolbar`, `StatusBar`
- **Docked editor windows** (`Engine/Editor/EditorWindows/`): viewport, outliner, components hierarchy, details
- **Reusable UI components** (`Engine/Editor/UIComponents/`): class picker popup, context menus, toggle groups, type chips, and property widgets (`ScalarField`, `StringField`, `Vec3Field`, `ColorField`, `ReferenceField`)

`EditorRenderManager` draws the fixed top/bottom panels first, then hosts the main dockspace between them.

### Editor Windows (`Engine/Editor/EditorWindows/`)

All editor windows implement `EditorWindow` interface. Current windows:
- `EditorWindow_Viewport` — displays scene texture, fly camera (WASD + mouse); supports viewport presets (`EditorWindow_ViewportPresets.h`); renders **ImGuizmo** transform gizmos on the selected `SceneComponent` (translate/rotate/scale), creating an undo entry on release
- `EditorWindow_WorldOutliner` — scene hierarchy tree
- `EditorWindow_ComponentsHierarchy` — components of selected GameObject
- `EditorWindow_Details` — property inspector (reflection-driven: reads DClass/DProperty to display fields; fires `WidgetEditEvent` on edits, dispatching `EditorCommand_SetProperty`; also shows `DFUNCTION()`-annotated methods as invocable buttons)
- `EditorWindow_AssetBrowser` — folder tree of imported assets with select/duplicate/delete actions

`EditorSelectionState` is the shared selection model; it is owned by `EditorCore` (not `EditorMain`) and accessed via `EditorCore::GetSelectionState()`.

### EditorCore (`Engine/Editor/EditorCore.h`)

`EditorCore` is the central editor state object (global `g_editorCore` pointer). It owns and initialises:
- `EditorAssetDatabase` — asset registry
- `EditorSelectionState` — selection model
- `EditorCommandManager` — undo/redo stack

Key operations exposed:
- `LoadScene(path)`, `GetWorld()`, `GetActiveSceneAsset()`
- `ResolveObject(assetId, objectId)` / `GetIdsForObject(obj)` — bidirectional UUID↔pointer lookup
- `SubmitMcpRequest(json)` / `DrainMcpRequests()` — main-thread MCP request pump: the socket thread submits a raw line and blocks on the returned future; `DrainMcpRequests()` (called once per frame on the main thread) routes each and fulfils the promise
- Supports headless mode (`Initialize(..., headless=true)`) for test environments

### Editor Command System (`Engine/Editor/Commands/`)

A full undo/redo command system built on the Command pattern:

| Class | Role |
|---|---|
| `EditorCommand` | Abstract base; `Execute / Undo / Redo / Serialize / Deserialize` |
| `EditorCommandManager` | Owns undo/redo stacks (max 100 deep) |
| `EditorCommandRegistry` | Factory registry; `CommandRegistrar<T>` auto-registers at startup |
| `EditorCommandContext` | Carries `EditorCore&` through every command call |
| `EditorCommandBatch` | Groups multiple commands into one undoable unit |
| `EditorAuxiliaryCommand` | Non-undoable side effects (e.g. selection changes) |

Concrete commands:
- `EditorCommand_CreateGameObject` / `EditorCommand_DeleteGameObject`
- `EditorCommand_CreateComponent` / `EditorCommand_DeleteComponent`
- `EditorCommand_SetProperty` — uses `PropertyValueIO` to read/write reflected property values as JSON; works with any `DProperty`
- `EditorCommand_RenameObject` / `EditorCommand_ReparentSceneComponent`
- `EditorAuxiliarySceneCommands` — auxiliary (non-undoable) scene operations

Commands serialize to/from JSON, enabling undo-stack persistence and replay across sessions.

### MCP (Model Context Protocol) Integration

DeltaEngine ships a full MCP bridge that lets AI agents (Claude Code, etc.) query and manipulate the live editor over a local TCP socket.

**Terminology:** the MCP stack uses three terms consistently:
- **query** — read-only operation; never mutates project state
- **command** — mutating operation (may also return data); each command is its own undo entry where `undoable: true`
- **operation** — umbrella term covering both queries and commands

**C++ side (`Engine/Editor/Mcp/` + `Engine/Editor/McpSocketServer.h`):**

- `McpSocketServer` — async TCP server (Asio) on its own thread; listens on port 57340 by default; single client. It is pure transport: it never touches the engine object graph. For each newline-delimited line it calls a handler that submits the request to the main thread (`EditorCore::SubmitMcpRequest`) and blocks on the result, then writes back the single response line.
- **Threading model** — MCP queries and commands execute **synchronously on the editor main thread**. `EditorCore::DrainMcpRequests()` runs once per frame (before `Tick`/`TickGC`), routing each queued request through `McpQueryRouter::Route` and fulfilling its promise. This keeps all engine/graph/GC access single-threaded; there is no cross-thread access and no async command queue. Each operation returns one response line (`request_id` echoed when supplied); there is no accept/result two-phase protocol.
- `IMcpSystem` — interface for a named system that registers tools into `McpRegistry`.
- `McpQueryRouter` — routes incoming JSON envelopes. Queries: `{ "type": "query", "system", "query", "params" }`. Commands: `{ "type": "command", "system", "command", "params" }`. Each command handler constructs the concrete `EditorCommand` and runs it directly via `EditorCore::GetCommandManager()` (the shared `RunEditorCommand` helper in `McpProtocol` formats the `{ok, commandType, objectId}` result); there is no serialized-envelope round-trip.
- `McpRegistry` — holds separate query and command maps. `RegisterQuery` / `RegisterCommand` register handlers; `DispatchQuery` / `DispatchCommand` invoke them; `HasQuery`, `HasCommand`, `GetQueryNames`, `GetCommandNames` for introspection.
- **Systems** (`Engine/Editor/Mcp/Systems/`): `McpSceneSystem`, `McpAssetsSystem`, `McpReflectionSystem`, `McpSelectionSystem`, `McpUndoSystem`, `McpViewportSystem`, `McpProjectSystem`, `McpMetaSystem`, `McpCommonSystem`, `McpLightsSystem`.
- **Transform commands** in `McpSceneSystem` are per-channel: `SetPosition`, `SetRotation`, `SetScale`. Each writes the matching decomposed `SceneComponent` property (`m_localPosition` / `m_localRotation` or `m_localEulerAngles` / `m_localScale`) through `EditorCommand_SetProperty`. Each accepts an optional `duration_seconds` for tweened animation, defaulting to `kDefaultAnimationDurationSeconds` (1.0); `duration_seconds <= 0` applies immediately. There is no bulk `SetTransform` command. `SetPosition` / `SetRotation` take `space` (`local`/`world`); `SetScale` is local-only. `SetRotation` accepts a 4-element quaternion or a 3-element euler triple **in degrees** — a local-space euler is written to `m_localEulerAngles` verbatim so the caller's angles survive round-tripping.
- **Transform queries** `get_position` / `get_rotation` / `get_scale` read one channel with a `space` (`local`/`world`) option; `get_rotation` returns both `quaternion` and `euler` (degrees). The generic `component` query still returns all four decomposed properties.
- **Light intensity animation** in `McpLightsSystem` uses `SetIntensity` with the same `duration_seconds` pattern. Easing is fixed at `CubicEaseOut` (`TransformAnimationTypes.h`) — there is no `easing` parameter.
- **`EditorAnimationManager`** (`Engine/Editor/Animation/`) is owned by `EditorCore` and ticked between `EngineMain::Tick()` and `RenderFrame()`. It manages in-flight tween instances (`EditorAnimationInstance`) and per-object transform sessions (`TransformAnimationSession`). When every channel of a transform session finishes it commits one `EditorCommand_SetProperty` per participating decomposed property, batched into a single `"Animate Transform"` undo entry; scalar channels (intensity / other floats) commit individually. Not created in headless mode; `StartTransformChannelAnimation` then applies the target value and commits that channel's property immediately instead.

**Python side (`Tools/DeltaMCP/`):**

- `delta_mcp_server.py` — FastMCP server. Exposes three MCP tools:
  - `list_operations` — returns `{ systems: { "<system>": { queries: [...], commands: [...] } } }`.
  - `describe_operations` — takes `targets: [{system, query} | {system, command}, ...]` and returns `{ queries: {...}, commands: {...} }` with full param schemas.
  - `execute_batch` — executes a list of entries sequentially. Each entry is either a query envelope `{type:"query", system, query, params}` or a command envelope `{type:"command", system, command, params}`.
- Schema JSON files live in `Tools/DeltaMCP/Schemas/` (one per system). Each file has `"queries": {...}` and `"commands": {...}` top-level keys.
- MCP tests are in `Engine/Tests/Editor/Mcp/` (one file per system) using `McpCoreFixture`.

**Workflow for AI callers:** `list_operations` → `describe_operations` → run queries for IDs → `execute_batch` with commands.

---

## Logging System (`Engine/Runtime/Logging/`)

Structured logging built on **spdlog**, following UE5 conventions:

- **`DLogCategory`** — named category with per-category `ELogLevel` (`VeryVerbose` → `Fatal`); categories register themselves globally and can be reinitialized with new sinks.
- **`LoggingManager`** — call `Initialize(logDir)` once at startup and `Shutdown()` at exit; supports adding sinks at runtime (e.g. an in-editor console sink) and setting a global level override.
- **`LogChannels.h/.cpp`** — predefined engine-wide log channels.

Macros:
```cpp
DECLARE_LOG_CATEGORY(LogFoo)          // .h — forward-declares the category
DEFINE_LOG_CATEGORY(LogFoo)           // .cpp — defines it with default Log level
DEFINE_LOG_CATEGORY_STATIC(LogFoo)    // .cpp — file-local category

DLOG(LogFoo, ELogLevel::Warning, "Mesh {} failed to load", meshName);
DLOG_IF(LogFoo, ELogLevel::Error, cond, "detail: {}", val);
```

---

## Object Snapshots (`Engine/Runtime/Serialization/`)

`ObjectSnapshot` captures a partial or full serialized image of a `DObject` subtree for undo/redo:

- **`ObjectSnapshotWriter`** — writes a `DObject` (and selected properties) into an `ObjectSnapshot` (JSON + captured `ObjectId` list).
- **`ObjectSnapshotReader`** — applies a snapshot back onto an existing `DObject`, restoring only the captured fields without a full reload.
- Used by the editor command system (e.g. `EditorCommand_SetProperty`) to save/restore property state around undoable edits.

---

## Tests (`Engine/Tests/`)

A GTest-based test suite, built as a separate CMake target. Layout:

```
Engine/Tests/
├── Engine/           # Runtime library tests
│   ├── Reflection/   — ReflectionRegistry
│   ├── Serialization/— core serialization
│   ├── Assets/       — DPrimaryAsset, scene asset integration
│   ├── Core/         — GC system (DObjectRegistry, GCManager, sweep)
│   └── Graphics/     — render structure, render core, render graph compile
│         └── Gpu/    — GPU-level tests requiring a real D3D12 device
├── Editor/           # Editor library tests
│   ├── EditorCommandTests_UndoStack.cpp
│   ├── EditorCommandTests_SceneStructure.cpp
│   ├── EditorCommandTests_Scene_SaveLoad.cpp
│   ├── EditorCommandTests_SetProperty.cpp
│   ├── EditorCommandTests_CommandQueue.cpp
│   ├── EditorCommandTests_McpPath.cpp
│   ├── EditorCoreFixture.h   — shared headless EditorCore setup
│   ├── Mcp/          — per-system MCP tests (McpCoreFixture + one file per system)
│   ├── State/        — EditorSelectionState
│   ├── EditorWindows/— window concept + viewport preset tests
│   ├── Serialization/— snapshot round-trip, bulk data, references
│   └── UI/           — editor theme color tests
└── Shared/           — shared test helpers (SerializationTestSupport, TestEnvironment,
                        GpuGraphicsFixture, GpuTestAssetFixture, GpuReadback,
                        GpuSceneBuilder, GpuD3D12Validation)
```

`EditorCoreFixture` spins up `EditorCore` in headless mode so editor command tests run without a window or GPU.

`GpuGraphicsFixture` (in `Engine/Tests/Shared/`) initialises a real D3D12 device and direct queue for GPU-level tests. `GpuTestAssetFixture` extends it with a preloaded scene. `GpuReadback` provides CPU-side readback helpers. `GpuD3D12Validation` enables the D3D12 debug layer with break-on-error. Use these only for tests that genuinely need the GPU — they are slower and require the hardware to be available.

`DeltaHeaderTool` has its own **pytest** suite under `Tools/DeltaHeaderTool/tests/` (pytest installed in the bundled Python). Run it via `Tools\Scripts\DeltaCmd.bat --automatic test header-tool` from any tool/agent context.

`DeltaCmd` has a **pytest** self-test suite under `Tools/DeltaCmd/tests/`. Run it via `Tools\Scripts\DeltaCmd.bat --automatic test delta-cmd` (or `Tools\Scripts\test-delta-cmd.bat --automatic`).

---

## Reflection System

DeltaEngine has a full static reflection system. Source classes annotated with macros are processed at build time by `DeltaHeaderTool` to generate registration code. At runtime, `ReflectionRegistry` provides type lookup, property access, function invocation, and object creation by name.

### Annotation Macros (`Engine/Runtime/Macros.h`)

| Macro | Target | Effect |
|---|---|---|
| `DCLASS()` | class | Marks for reflection; DeltaHeaderTool generates `.generated.h/.cpp` |
| `DSTRUCT()` | struct | Same as DCLASS, generates DStruct metadata instead of DClass |
| `DENUM()` | `enum class` | Marks an enum for metadata registration (name, underlying type, enumerator list); expands to nothing at compile time |
| `DPROPERTY()` | field | Reflects the field; type and offset captured via AST. Optional tag `EditorOnly` (`DPROPERTY(EditorOnly)`) sets `DProperty::bEditorOnly = true`, hiding the property from runtime serialization |
| `DFUNCTION()` | method | Reflects the method; thunk + params struct generated |
| `DGENERATED_BODY(Name)` | class body | Injects `friend` declaration and `GetClass()` override |

All annotation macros expand to nothing at compile time — they are only tokens for the code generator.

### Reflection Types (`Engine/Runtime/Reflection/`)

- **`DStruct`** — metadata for structs: name, super name, size, alignment, linked list of `DProperty`
- **`DClass`** — extends `DStruct` for classes: adds `DFunction` map, constructor/destructor/copy lambdas, abstract flag
- **`DProperty`** — abstract base for field metadata; offset-based access; concrete subclasses include scalar/string/math types plus `DObjectPtrProperty<T>`, `DBulkDataProperty<T>`, `DVectorProperty<T>`, and `DEnumProperty<T>` (`EPropertyType::Enum`)
- **`DEnum`** / **`DEnumEntry`** — enum metadata: name, underlying type, and `{name, value}` enumerator list; registered via `ReflectionRegistry::RegisterDEnum` / looked up with `FindEnumByName`
- **`DFunction`** — method metadata: name, native thunk pointer, param list (`DProperty*`), optional return property; `Invoke(DObject*, void*)` dispatches via thunk
- **`ReflectionRegistry`** — singleton (`GetReflectionRegistry()`); maps name → `DStruct*` / `DClass*` / `DEnum*`; `CreateObject(name)` and `DestroyObject()` for runtime instantiation

### How Reflection Registration Works

Each reflected class gets a generated `.cpp` that:
1. Defines a **thunk function** per `DFUNCTION()` method — signature `void Thunk(DObject*, void*)`, extracts params from a generated struct, calls the real method, writes return value back.
2. Defines `ReflectionRegisterFn_ClassName()` — creates `DClass`/`DStruct`, adds `DProperty` instances (via `offsetof`), adds `DFunction` instances (with thunks), registers into `GetReflectionRegistry()`.
3. Defines `CreateDObject<ClassName>()` specialization — delegates to `ReflectionRegistry::CreateObject`.
4. Declares a **static `ReflectionRegistration`** instance — calls the register function at program startup, before `main()`.

`ReflectionRegistry::FinalizeRegistration()` is called once after startup to resolve super-class links across the full hierarchy.

### Generated File Layout

```
Source:     Engine/Runtime/Core/Foo.h          (annotated with DCLASS)
Generated:  Intermediate/DeltaHeaderTool/Generated/Foo.generated.h
            Intermediate/DeltaHeaderTool/Generated/Foo.generated.cpp
```

`Foo.generated.h` is `#include`d at the top of `Foo.h` (before the class body) to expose forward declarations required by `DGENERATED_BODY`. The generated `.cpp` files are compiled as part of `DeltaEngine` via the manifest.

### Enum Reflection (`DENUM()`)

- **`enum class` only** — unscoped enums are rejected by DeltaHeaderTool.
- Annotate the enum with `DENUM()` and use it as a `DPROPERTY()` field type; codegen maps the field to `DEnumProperty<T>` and registers `DEnum` metadata at startup.
- **Serialization** — assets, undo JSON, and MCP all read/write the **underlying integer** (e.g. `"m_renderMode": 2`), not enumerator name strings.
- **Editor** — the Details panel renders an ImGui combo built from `DEnum` metadata (enumerator display names, integer wire values).
- **`std::vector<EnumType>`** is supported for enum properties.
- **Production example:** `DMaterial::m_renderMode` is `ERenderMode` (`Opaque=0`, `Masked=1`, `Transparent=2`).
- **Non-goals (initial rollout):** unscoped enum, string-based enum serialization, `DFUNCTION()` enum params.

### Currently Reflected Classes (40)

Core / Scene: `DObject`, `GameObject`, `DComponent`, `SceneComponent`, `DWorld`, `Camera`, `DScene`
Skybox: `Skybox`
Rendering: `Renderer`, `MeshRenderer` (DSTRUCT), `DMesh`, `DMaterial` (PBR properties: baseColor, metallic, roughness, emissive; `m_renderMode` as `ERenderMode` enum; flags via `MaterialFlags`), `DTexture`, `DShader`
Post-processing: `PostProcessPass` (abstract), `PostProcessStack`, `PassthroughPass`, `TonemapPass`
Assets: `DPrimaryAsset`, `PA_DScene`, `PA_Shader`, `PA_Material`, `PA_Texture`, `PA_StaticMesh`, `PA_Skybox`, `PA_PostProcessStack`
Lighting: `LightComponent`, `DirectionalLight`, `PointLight`, `SpotLight`
Test / Serialization: `TestComponent`, `TestComponent2`, `DTestObjectA`, `DTestObjectB`, `PA_TestAsset`, `DTestMeshData`, `PA_TestMesh`, `DSnapshotTestComponentA`, `DSnapshotTestComponentB`, `DSnapshotTestSceneComponent`

### Reflection Limitations

- `std::vector<T>` properties are supported, including nested vectors such as `std::vector<std::vector<float>>`.
- Vector elements may be value types, reflected raw pointers (`T*`), nested vectors, or reflected `enum class` types.
- Template class reflection is **not** supported.
- Nested class reflection is **not** supported.
- Method overloads are tracked by index but discrimination is limited.

---

## Tools & Scripts

### Directory Layout

```
Tools/
├── Python/              # Bundled Python 3.12+ (build-time only, not embedded at runtime)
│   ├── python.exe
│   ├── Lib/
│   └── DLLs/
├── Clang/               # LLVM libclang.dll + Python bindings (used by DeltaHeaderTool)
├── DeltaMCP/            # Python FastMCP server for AI-agent editor control
│   ├── delta_mcp_server.py  # MCP tools: list_operations, describe_operations, execute_batch
│   └── Schemas/             # JSON schemas per system (scene.json, assets.json, …)
├── DeltaHeaderTool/     # Reflection code generator (~1,900 lines Python)
│   ├── main.py          # Driver: two-pass pipeline, multiprocessing pool
│   ├── parser.py        # libclang AST parser → ClassInfo / PropertyInfo / FunctionInfo
│   ├── generator.py     # Code emitter → .generated.h and .generated.cpp
│   ├── templates.py     # String templates for thunks, registration, property binding
│   ├── type_resolver.py # C++ type → DProperty subclass mapping
│   ├── diagnostics.py   # Non-fatal warning accumulation
│   └── clang/           # Bundled libclang Python bindings
├── DeltaCmd/            # Unified build/run/test CLI (bundled Python)
│   ├── main.py          # Entry point
│   ├── cli.py           # argparse subcommands
│   ├── env.py           # repo root, bundled python, VS devcmd, --automatic
│   ├── registry.py      # presets, targets, paths (single source of truth)
│   ├── configure.py
│   ├── build.py
│   ├── run.py
│   ├── test.py
│   ├── header.py
│   ├── list_cmd.py
│   └── tests/           # pytest self-tests
├── CMake/
│   └── PythonSetup.cmake  # Sets DELTA_PYTHON to bundled python.exe
└── Scripts/
    ├── DeltaCmd.bat                     # Single forwarder to Tools/DeltaCmd/main.py
    ├── build.bat
    ├── build-x64-debug.bat                # → build editor
    ├── build-x64-debug-engine-tests.bat   # → build engine-tests
    ├── build-x64-debug-editor-tests.bat   # → build editor-tests
    ├── configure-x64-debug.bat            # → configure
    ├── rebuild-x64-debug.bat              # → configure + build editor
    ├── run-x64-debug.bat                  # → run editor
    ├── run-x64-debug-engine-tests.bat     # → test engine
    ├── run-x64-debug-editor-tests.bat     # → test editor
    ├── set_env.bat
    ├── delta_header_generate.bat          # → header generate
    ├── delta_header_force_generate.bat    # → header force
    ├── test-delta-header-tool.bat         # → test header-tool
    └── test-delta-cmd.bat                 # → test delta-cmd

    # All scripts pause at the end when run interactively.
    # Pass --automatic when invoking from tools/agents/CI.
```

### DeltaHeaderTool Pipeline

**Two-pass architecture:**

1. **Fast text scan** — quick grep for `DCLASS`/`DSTRUCT` tokens; skips unchanged files by timestamp.
2. **Parallel AST parse** — `multiprocessing.Pool` (default: all CPU cores); each worker loads libclang once, strips `#include` directives, parses with `-std=c++23`, walks AST to extract annotated classes, properties, and functions.

**Output per source file:**
- `.generated.h` — params structs, `ReflectionRegister_*` forward declarations, `CreateDObject<>` specialization
- `.generated.cpp` — thunk functions, full registration function, `CreateDObject<>` implementation, static registration instance

**Key design:** libclang parses a stripped copy of the header (includes removed, replaced with minimal type stubs) so the tool has zero dependency on the full engine include tree.

### Python Usage

Python is **build-time only**. The bundled `Tools/Python/python.exe` runs `DeltaHeaderTool` during CMake builds and also provides `pytest` for the DeltaHeaderTool test suite. There is **no embedded Python in the engine runtime** (no pybind11, no Python C API in `Engine/` code).

---

## Key Conventions

- **Headers only for declarations/implementations split:** most files use `.h` + `.cpp` pairs under the same directory.
- **Mixed ownership model:** subsystems use RAII smart pointers, while reflected `DObject` relationships and scene/component links are typically raw pointers.
- **No raw `new`/`delete`** for reflected engine objects — use `CreateDObject<T>()`; asset ownership/lifetime is handled by reflection registry + `DPrimaryAsset`.
- **Slang shaders** live in `Engine/EngineSourceAssets/Shaders/` as `.slang` files and are compiled at runtime via `CompileSlangStage`. The `StandardObject.slang` / `StandardLighting.slang` / `StandardConstantStructs.slang` / `StandardInputs.slang` quartet forms the standard material shader. `PBRObject.slang` / `PBRLighting.slang` / `PBRInputs.slang` form the PBR material shader set (uses IBL).
- **`DXGraphicsContext`** is the primary way to pass rendering state down the call stack — do not add global graphics state.
- **Adding a new reflected class:** annotate with `DCLASS()` + `DGENERATED_BODY(Name)`, add `DPROPERTY()`/`DFUNCTION()` annotations, then build (or run `DeltaCmd.bat --automatic header generate`) — the tool regenerates the `.generated.h/.cpp` pair automatically.
- **Do not hand-edit generated files** in `Intermediate/DeltaHeaderTool/Generated/` — they are overwritten on every build.
- **Adding a new editor command:** subclass `EditorCommand`, implement `Execute`/`Undo`/`Serialize`/`Deserialize`, add a `static constexpr std::string_view StaticTypeName()`, and declare a `CommandRegistrar<T>` static instance in the `.cpp` to auto-register with `EditorCommandRegistry`.
- **Logging:** use `DLOG(Category, ELogLevel::X, ...)` everywhere; define a `DEFINE_LOG_CATEGORY` in the `.cpp` and `DECLARE_LOG_CATEGORY` in the `.h`.
- **Property edits in the editor** must go through `EditorCommand_SetProperty` (not direct assignment) so they are undoable.
