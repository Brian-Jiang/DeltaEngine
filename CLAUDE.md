# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DeltaEngine is a DirectX 12 game engine with an integrated editor, written in C++23. The editor uses ImGui for UI and SDL3 for window management. The engine has a full static reflection system driven by a Python-based code generator (`DeltaHeaderTool`).

## Build System

**CMake + Ninja**, targeting Windows x64. The only configured preset is `x64-debug`.

**Always drive builds, runs, and tests through `Tools/Scripts/*.bat`** — they set up `DELTA_PROJECT_ROOT`, the bundled Python, and the VS dev environment. Do not call `cmake` / `pytest` directly from tool or agent invocations.

```bash
Tools\Scripts\build-x64-debug.bat              # incremental build (DeltaEditorLaunch)
Tools\Scripts\rebuild-x64-debug.bat            # configure + build (DeltaEditorLaunch)
Tools\Scripts\build-x64-debug-engine-tests.bat # build DeltaEngineTests
Tools\Scripts\build-x64-debug-editor-tests.bat # build DeltaEditorTests
Tools\Scripts\run-x64-debug.bat                # launch DeltaEditorLaunch.exe
Tools\Scripts\delta_header_generate.bat        # incremental reflection codegen
Tools\Scripts\delta_header_force_generate.bat  # full reflection codegen
Tools\Scripts\test-delta-header-tool.bat       # pytest for DeltaHeaderTool
```

### `--automatic` flag (MANDATORY for tool/agent invocations)

Every script in `Tools/Scripts/` pauses at the end by default so a human double-clicking the `.bat` can read the output. **When invoked by Claude Code, an agent, CI, or any other non-interactive context, always pass `--automatic`** so the script skips the trailing `pause` and returns the real exit code. Omitting it will hang the tool call forever waiting on a keypress.

```bash
Tools\Scripts\build-x64-debug.bat --automatic
Tools\Scripts\test-delta-header-tool.bat --automatic
Tools\Scripts\test-delta-header-tool.bat --automatic -k test_parser  # extra args pass through to pytest
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

# Or via script
Tools/Scripts/delta_header_force_generate.bat
```

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
└── GameObject[]
      ├── SceneComponent (root transform, parent-child tree)
      │     ├── MeshRenderer  → DMesh + DMaterial[]
      │     ├── Camera
      │     ├── DirectionalLight / PointLight / SpotLight
      │     └── (nested SceneComponents)
      └── DComponent  (non-spatial components)
```

- `DObject` is the base for everything. `GameObject`, `DComponent`, `SceneComponent` all inherit from it.
- `DObject`-derived relationships in reflected/runtime gameplay data are represented with raw pointers (`T*`).
- Asset ownership is centralized in `DPrimaryAsset`, which stores loaded objects as `std::shared_ptr<DObject>` and returns raw pointers for lookup/use.
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

- **Models:** `DMesh::Initialize(...)` currently imports directly via Assimp and builds submesh/material/texture data.
- **Textures:** `DTexture` initialization currently goes through `TextureImporter`; the standalone importer layer exists in `Engine/Runtime/Importers/` but is not an active high-level pipeline right now.
- **Shaders:** `DShader` compiles HLSL at runtime using DXC (`dxcompiler.dll`). Sources in `Engine/Runtime/Shaders/`.

### Editor UI (`Engine/Editor/`)

The editor now has a fixed-position chrome layer plus docked content windows:

- **Fixed panels** (`Engine/Editor/Panels/`): `AppHeader`, `MainToolbar`, `StatusBar`
- **Docked editor windows** (`Engine/Editor/EditorWindows/`): viewport, outliner, components hierarchy, details
- **Reusable UI components** (`Engine/Editor/UIComponents/`): class picker popup, context menus, toggle groups, type chips, and property widgets (`ScalarField`, `StringField`, `Vec3Field`, `ColorField`, `ReferenceField`)

`EditorRenderManager` draws the fixed top/bottom panels first, then hosts the main dockspace between them.

### Editor Windows (`Engine/Editor/EditorWindows/`)

All editor windows implement `EditorWindow` interface. Current windows:
- `EditorWindow_Viewport` — displays scene texture, fly camera (WASD + mouse); supports viewport presets (`EditorWindow_ViewportPresets.h`)
- `EditorWindow_WorldOutliner` — scene hierarchy tree
- `EditorWindow_ComponentsHierarchy` — components of selected GameObject
- `EditorWindow_Details` — property inspector (reflection-driven: reads DClass/DProperty to display fields; fires `WidgetEditEvent` on edits, dispatching `EditorCommand_SetProperty` through the command system)
- `EditorWindow_AssetBrowser` — folder tree of imported assets with select/duplicate/delete actions

`EditorSelectionState` is the shared selection model; it is owned by `EditorCore` (not `EditorMain`) and accessed via `EditorCore::GetSelectionState()`.

### EditorCore (`Engine/Editor/EditorCore.h`)

`EditorCore` is the central editor state object (global `g_editorCore` pointer). It owns and initialises:
- `EditorAssetDatabase` — asset registry
- `EditorSelectionState` — selection model
- `EditorCommandManager` — undo/redo stack

Key operations exposed:
- `LoadScene(path)`, `GetWorld()`, `GetActiveSceneAsset()`
- `CreateGameObject(name)`, `AddComponentToGameObject(id, className)`
- `ResolveObject(assetId, objectId)` / `GetIdsForObject(obj)` — bidirectional UUID↔pointer lookup
- `EnqueueSerializedCommand(json)` + `DrainCommandQueue()` — serialized command dispatch for headless / MCP callers
- Supports headless mode (`Initialize(..., headless=true)`) for test environments

### Editor Command System (`Engine/Editor/Commands/`)

A full undo/redo command system built on the Command pattern:

| Class | Role |
|---|---|
| `EditorCommand` | Abstract base; `Execute / Undo / Redo / Serialize / Deserialize` |
| `EditorCommandManager` | Owns undo/redo stacks (max 100 deep); `SerializeUndoStack` / `DeserializeAndReplay` |
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
│   └── Graphics/     — render structure, render core
├── Editor/           # Editor library tests
│   ├── EditorCommandTests_UndoStack.cpp
│   ├── EditorCommandTests_SceneStructure.cpp
│   ├── EditorCommandTests_Scene_SaveLoad.cpp
│   ├── EditorCommandTests_SetProperty.cpp
│   ├── EditorCommandTests_CommandQueue.cpp
│   ├── EditorCoreFixture.h   — shared headless EditorCore setup
│   ├── State/        — EditorSelectionState
│   ├── EditorWindows/— window concept + viewport preset tests
│   ├── Serialization/— snapshot round-trip, bulk data, references
│   └── UI/           — editor theme color tests
└── Shared/           — shared test helpers (SerializationTestSupport, TestEnvironment)
```

`EditorCoreFixture` spins up `EditorCore` in headless mode so editor command tests run without a window or GPU.

`DeltaHeaderTool` has its own **pytest** suite under `Tools/DeltaHeaderTool/tests/` (pytest installed in the bundled Python). Run it via `Tools\Scripts\test-delta-header-tool.bat --automatic` from any tool/agent context.

---

## Reflection System

DeltaEngine has a full static reflection system. Source classes annotated with macros are processed at build time by `DeltaHeaderTool` to generate registration code. At runtime, `ReflectionRegistry` provides type lookup, property access, function invocation, and object creation by name.

### Annotation Macros (`Engine/Runtime/Macros.h`)

| Macro | Target | Effect |
|---|---|---|
| `DCLASS()` | class | Marks for reflection; DeltaHeaderTool generates `.generated.h/.cpp` |
| `DSTRUCT()` | struct | Same as DCLASS, generates DStruct metadata instead of DClass |
| `DPROPERTY()` | field | Reflects the field; type and offset captured via AST |
| `DFUNCTION()` | method | Reflects the method; thunk + params struct generated |
| `DGENERATED_BODY(Name)` | class body | Injects `friend` declaration and `GetClass()` override |

All annotation macros expand to nothing at compile time — they are only tokens for the code generator.

### Reflection Types (`Engine/Runtime/Reflection/`)

- **`DStruct`** — metadata for structs: name, super name, size, alignment, linked list of `DProperty`
- **`DClass`** — extends `DStruct` for classes: adds `DFunction` map, constructor/destructor/copy lambdas, abstract flag
- **`DProperty`** — abstract base for field metadata; offset-based access; concrete subclasses include scalar/string/math types plus `DObjectPtrProperty<T>`, `DBulkDataProperty`, and `DVectorProperty<T>`
- **`DFunction`** — method metadata: name, native thunk pointer, param list (`DProperty*`), optional return property; `Invoke(DObject*, void*)` dispatches via thunk
- **`ReflectionRegistry`** — singleton (`GetReflectionRegistry()`); maps name → `DStruct*` / `DClass*`; `CreateObject(name)` and `DestroyObject()` for runtime instantiation

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

### Currently Reflected Classes (24)

Core / Scene: `DObject`, `GameObject`, `DComponent`, `SceneComponent`, `DWorld`, `Camera`
Rendering / Assets: `Renderer`, `MeshRenderer`, `DMesh`, `DMaterial`, `DTexture`, `DShader`, `DPrimaryAsset`
Lighting: `LightComponent`, `DirectionalLight`, `PointLight`, `SpotLight`
Test / Serialization: `TestComponent`, `TestComponent2`, `DTestObjectA`, `DTestObjectB`, `PA_TestAsset`, `DTestMeshData`, `PA_TestMesh`

### Reflection Limitations

- `std::vector<T>` properties are supported, including nested vectors such as `std::vector<std::vector<float>>`.
- Vector elements may be value types, reflected raw pointers (`T*`), or nested vectors.
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
├── DeltaHeaderTool/     # Reflection code generator (~1,900 lines Python)
│   ├── main.py          # Driver: two-pass pipeline, multiprocessing pool
│   ├── parser.py        # libclang AST parser → ClassInfo / PropertyInfo / FunctionInfo
│   ├── generator.py     # Code emitter → .generated.h and .generated.cpp
│   ├── templates.py     # String templates for thunks, registration, property binding
│   ├── type_resolver.py # C++ type → DProperty subclass mapping
│   ├── diagnostics.py   # Non-fatal warning accumulation
│   └── clang/           # Bundled libclang Python bindings
├── CMake/
│   └── PythonSetup.cmake  # Sets DELTA_PYTHON to bundled python.exe
└── Scripts/
    ├── build.bat
    ├── build-x64-debug.bat                # Build DeltaEditorLaunch
    ├── build-x64-debug-engine-tests.bat   # Build DeltaEngineTests
    ├── build-x64-debug-editor-tests.bat   # Build DeltaEditorTests
    ├── rebuild-x64-debug.bat              # Configure + build DeltaEditorLaunch
    ├── run-x64-debug.bat                  # Launch DeltaEditorLaunch.exe
    ├── set_env.bat
    ├── delta_header_generate.bat          # Incremental generation
    ├── delta_header_force_generate.bat    # Full regeneration (--force)
    └── test-delta-header-tool.bat         # pytest for DeltaHeaderTool

    # All scripts pause at the end when run interactively.
    # Pass --automatic (first arg) when invoking from tools/agents/CI.
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
- **HLSL shaders** live alongside engine source in `Engine/Runtime/Shaders/` and are compiled at runtime (not offline). The `StandardObject.hlsl` / `StandardLighting.hlsl` / `StandardConstantStructs.hlsl` trio forms the standard material shader.
- **`DXGraphicsContext`** is the primary way to pass rendering state down the call stack — do not add global graphics state.
- **Adding a new reflected class:** annotate with `DCLASS()` + `DGENERATED_BODY(Name)`, add `DPROPERTY()`/`DFUNCTION()` annotations, then build (or run `delta_header_generate.bat`) — the tool regenerates the `.generated.h/.cpp` pair automatically.
- **Do not hand-edit generated files** in `Intermediate/DeltaHeaderTool/Generated/` — they are overwritten on every build.
- **Adding a new editor command:** subclass `EditorCommand`, implement `Execute`/`Undo`/`Serialize`/`Deserialize`, add a `static constexpr std::string_view StaticTypeName()`, and declare a `CommandRegistrar<T>` static instance in the `.cpp` to auto-register with `EditorCommandRegistry`.
- **Logging:** use `DLOG(Category, ELogLevel::X, ...)` everywhere; define a `DEFINE_LOG_CATEGORY` in the `.cpp` and `DECLARE_LOG_CATEGORY` in the `.h`.
- **Property edits in the editor** must go through `EditorCommand_SetProperty` (not direct assignment) so they are undoable.
