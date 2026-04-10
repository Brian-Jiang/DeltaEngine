---
paths:
  - "Engine/**/*.cpp"
  - "Engine/**/*.h"
---
# API Export Macros

> This file **extends** [cpp/patterns.md](../cpp/patterns.md) with DeltaEngine DLL export conventions.

Use `DELTAENGINE_API` for runtime types and `DELTAEDITOR_API` for editor types.

**Prefer per-method export over whole-class export** unless the entire class needs to be visible across the DLL boundary.

```cpp
// Preferred — export only what crosses the boundary
class MeshRenderer : public Renderer
{
public:
    DELTAENGINE_API void GatherDrawCalls(DXGraphicsContext& ctx);
    DELTAENGINE_API static MeshRenderer* Create();
};

// Acceptable when all public members must be exported
class DELTAENGINE_API DObject
{
    ...
};
```

Avoid exporting whole classes that have private implementation details not intended for external use.
