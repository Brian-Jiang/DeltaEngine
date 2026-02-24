---
name: deltaengine-patterns
description: Coding patterns and conventions extracted from the DeltaEngine C++23 DirectX 12 game engine
version: 1.0.0
source: local-git-analysis
analyzed_commits: 120
---

# DeltaEngine Patterns

## Commit Conventions

Informal, topic-based — no conventional commit format:
- Short topic label: `Engine`, `Graphics`, `Dx12`, `Lights`, `Camera`, `Imgui`
- Fix prefix: `Fix <description>` (e.g. `Fix normal`, `Fix camera not update aspect ratio`)
- WIP suffix: `<Topic> (unfinish)` or `<Topic> (unfinished)` for incomplete commits

## Header File Template

```cpp
#pragma once
#include "EngineIncludes.h"   // Always first

#include <memory>
#include <DirectXMath.h>
#include "Runtime/Core/DObject.h"

DELTA_ENGINE_NS_BEGIN

class Foo;  // forward declare dependencies

class MyClass : public DObject
{
public:
    DELTAENGINE_API MyClass();
    DELTAENGINE_API void DoSomething();
    inline SomeType GetValue() const { return m_value; }  // trivial getters inline in header

private:
    SomeType m_value;   // m_ prefix on all private/protected members
};

DELTA_ENGINE_NS_END


Key Macros (Macros.h)

┌───────────────────────┬─────────────────────────────────┐
│         Macro         │           Expands to            │
├───────────────────────┼─────────────────────────────────┤
│ DELTA_ENGINE_NS_BEGIN │ namespace DeltaEngine {         │
├───────────────────────┼─────────────────────────────────┤
│ DELTA_ENGINE_NS_END   │ }                               │
├───────────────────────┼─────────────────────────────────┤
│ DELTAENGINE_API       │ __declspec(dllexport/dllimport) │
└───────────────────────┴─────────────────────────────────┘

- EngineIncludes.h includes Macros.h and sets NOMINMAX
- DELTAENGINE_API on every public method; not on inline getters

Type Naming

┌───────────────┬───────────────────────────────────────────────────────────────────────────────────────┐
│    Prefix     │                                       Used for                                        │
├───────────────┼───────────────────────────────────────────────────────────────────────────────────────┤
│ D             │ Engine domain types: DObject, DComponent, DMesh, DMaterial, DTexture, DShader, DWorld │
├───────────────┼───────────────────────────────────────────────────────────────────────────────────────┤
│ DX            │ DirectX wrappers: DXRenderManager, DXGraphicsContext                                  │
├───────────────┼───────────────────────────────────────────────────────────────────────────────────────┤
│ Editor        │ Editor systems: EditorMain, EditorRenderManager, EditorSelectionState                 │
├───────────────┼───────────────────────────────────────────────────────────────────────────────────────┤
│ EditorWindow_ │ Editor windows: EditorWindow_Viewport, EditorWindow_WorldOutliner                     │
└───────────────┴───────────────────────────────────────────────────────────────────────────────────────┘

Smart Pointer Rules

- shared_ptr for all scene objects (inherit enable_shared_from_this where needed)
- unique_ptr for owned subsystems (single owner)
- weak_ptr for back-references (e.g. m_parent in SceneComponent)
- No raw new/delete

C++20 Concepts

template<typename T>
concept IsSceneComponent = std::derived_from<T, SceneComponent>;

template<typename T>
concept IsDComponent = std::derived_from<T, DComponent>;

Apply to factory/template APIs for compile-time type safety.

DXGraphicsContext Pattern

Pass as shared_ptr<DXGraphicsContext> through the entire render stack. Never add global graphics state — add it to the
context instead.

struct DXGraphicsContext {
    std::shared_ptr<DXRenderManager> renderManager;
    std::shared_ptr<Device> device;
    std::shared_ptr<CommandList> commandList;
    std::vector<DirectionalLightBuffer> directionalLights;
    // ...
};

// Renderer interface
void GatherDrawCalls(DXGraphicsContext& context);

DirectX Math Types

- Public API: DirectX::SimpleMath::Vector3/Quaternion — easier to use
- Internal storage: DirectX::XMMATRIX / DirectX::XMVECTOR — performance
- Always provide float x/y/z overloads alongside vector overloads

Adding a New Editor Window

1. Create Engine/Editor/EditorWindows/EditorWindow_<Name>.h/.cpp
2. Inherit EditorWindow; take EditorSelectionState& if selection needed
3. Register in EditorMain

Adding a New Renderer Component

1. Inherit SceneComponent, implement GatherDrawCalls(DXGraphicsContext&)
2. Create matching RenderProxy struct for GPU decoupling

File Organization

Engine/
├── Runtime/
│   ├── Core/         # D-prefixed domain types
│   ├── Graphics/
│   │   ├── DirectX/  # DX12 wrappers
│   │   ├── Renderer/ # MeshRenderer, SpriteRenderer
│   │   └── Structures/
│   ├── Importers/
│   ├── Shaders/      # HLSL (runtime-compiled by DXC)
│   ├── EngineMain.h/.cpp
│   ├── EngineIncludes.h
│   └── Macros.h
└── Editor/
    ├── EditorWindows/
    ├── EditorMain.h/.cpp
    ├── EditorRenderManager
    └── EditorSelectionState

Build

cmake --preset x64-debug
cmake --build Build/x64-Debug
# Output: Build/x64-Debug/bin/ and Build/x64-Debug/lib/
- C++23, MSVC /permissive- + /MP, single x64-debug preset
- Shaders compiled at runtime via DXC (not offline)

---

**Note:** A write hook in this project blocks creating `.md` files outside of `README`, `CLAUDE`, `AGENTS`,
`CONTRIBUTING`, and `.claude/plans/`. To save the skill, either add `.claude/skills/` to the hook's allowed paths, or
disable the hook temporarily.

**Patterns detected from 120 commits:**
- Informal commit style with `(unfinish)` WIP tags
- `DELTAENGINE_API` + `DELTA_ENGINE_NS_BEGIN/END` macro pattern on every public class
- `D` prefix naming convention for all engine domain types
- `shared_ptr`/`weak_ptr`/`unique_ptr` exclusively (no raw `new`)
- `DXGraphicsContext` as the frame-scoped render state carrier
- `SimpleMath` for API, raw `XMMATRIX` for internals
- Hot files: `EngineMain.cpp`, `DXRenderManager.cpp`, `MeshRenderer.cpp`