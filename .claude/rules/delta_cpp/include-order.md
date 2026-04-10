---
paths:
  - "Engine/**/*.cpp"
  - "Engine/**/*.h"
---
# Include / Import Order

> This file **extends** [cpp/coding-style.md](../cpp/coding-style.md) with DeltaEngine-specific include ordering.

## In `.cpp` files

```cpp
#include "Runtime/Graphics/Renderer/MeshRenderer.h"   // 1. Paired header (always first)

#include "Editor/EditorCore.h"                         // 2. Project headers
#include "Editor/Commands/EditorCommand_SetProperty.h" //    - Editor headers before Runtime headers
#include "Runtime/Core/DObject.h"                      //    - then Runtime headers
#include "Runtime/Reflection/DProperty.h"

#include <imgui.h>                                     // 3. Third-party headers
#include <nlohmann/json.hpp>

#include <string>                                      // 4. Standard library headers
#include <vector>
```

- Each group is separated by a **single blank line**.
- Within the project headers group, **Editor headers always come before Runtime headers**.
- No mixing groups.

## In `.h` files

```cpp
#pragma once                                           // 1. Always first line

#include "EditorIncludes.h"                            // 2. Umbrella include — use ONE of:
// #include "EngineIncludes.h"                         //    EditorIncludes.h  (editor-layer files)
                                                       //    EngineIncludes.h  (runtime-layer files)

#include "Runtime/Core/DObject.h"                     // 3. Engine headers
#include "Runtime/Reflection/DProperty.h"

#include <DirectXMath.h>                              // 4. Third-party headers

#include <string>                                      // 5. Standard library headers
#include <vector>

#include "MeshRenderer.generated.h"                   // 6. Generated header LAST (if reflected)
```

- Each group is separated by a **single blank line**. Omit the group entirely if it has no entries.
- Group 2 is always a single umbrella include — `EditorIncludes.h` for editor-layer files, `EngineIncludes.h` for runtime-layer files. Never include both.
- The `.generated.h` always goes **last**.
- Prefer **forward declarations** over includes in headers whenever only a pointer or reference is used.

## Include path root

All project includes must be rooted at the `Engine/` subdirectory level:

```cpp
// Correct
#include "Runtime/Core/DObject.h"
#include "Editor/EditorCore.h"

// Wrong — relative paths, ambiguous root
#include "DObject.h"
#include "../Core/DObject.h"
```
