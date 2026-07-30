---
paths:
  - "Engine/**/*.cpp"
  - "Engine/**/*.h"
---
# Logging

> This file **extends** [cpp/coding-style.md](../cpp/coding-style.md) with DeltaEngine's structured logging conventions.

Use the `DLOG` macro everywhere. Never use `printf`, `std::cout`, or raw spdlog calls.

## Log categories are module-private

A category names *the subsystem that emitted a line*. It is never exported across a DLL boundary, so **a module may only log into categories it defines itself**. The editor does not log into `LogRenderer`; a game module does not log into `LogCore`. Each module defines its own.

`DECLARE_LOG_CATEGORY` / `DEFINE_LOG_CATEGORY` carry no export macro, and there is no `_API` variant. A cross-module use is an `undeclared identifier` compile error.

## Defining a category

Default to a file-local category — no header, nothing exposed:

```cpp
// In the .cpp
DEFINE_LOG_CATEGORY_STATIC(LogMeshRenderer);

DLOG(LogMeshRenderer, ELogLevel::Warning, "Mesh {} failed to load", meshName);
DLOG_IF(LogMeshRenderer, ELogLevel::Error, ptr == nullptr, "Null pointer: {}", context);
```

`LogCategory.h` — the `DLOG` macros, `ELogLevel`, and `DLogCategory` — comes in through the umbrella include (`EngineIncludes.h` / `EditorIncludes.h`). **Never include it directly**; every translation unit already has it.

When several `.cpp` files in the **same module** share a category, declare it in a **module-private** header — one not reachable from another module's umbrella include. Follow the existing `*Log.h` convention (`Engine/Runtime/Graphics/Light/LightLog.h`, `Engine/Editor/EditorMainLog.h`):

```cpp
// LightLog.h — included only by the light .cpp files
#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

DECLARE_LOG_CATEGORY(LogLight)

DELTA_ENGINE_NS_END
```

```cpp
// LightComponent.cpp
DELTA_ENGINE_NS_BEGIN

DEFINE_LOG_CATEGORY(LogLight)

DELTA_ENGINE_NS_END
```

Never declare a category in a header that other modules include. `Engine/Runtime/Logging/LogChannels.h` holds the engine-wide channels (`LogCore`, `LogRenderer`, `LogAsset`, …) and is **engine-private** — it is deliberately the one logging header kept *out* of `EngineIncludes.h`, so engine `.cpp` files include it directly and no other module can reach it.

## Logging from header-inline or template code

Code in a header compiles into whichever module includes it, so it must not name a category. Route it through an exported free function instead — see `DeltaInternal::CheckFailed` (`Engine/Runtime/Assert/Assert.h`) and `LoggingInternal::LogArrayResizeFailure` (`Engine/Runtime/Logging/LoggingInternal.h`).

## Observing another module's categories

Reading or reconfiguring is allowed; logging into one is not. Look the category up by name rather than by symbol:

```cpp
if (DLogCategory* cat = DLogCategory::FindByName("LogEngine"))
    cat->SetLevel(ELogLevel::Verbose);
```

`LoggingManager::AddSink` and `SetGlobalLevel` already reach every registered category in every module.
