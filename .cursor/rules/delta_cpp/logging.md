---
paths:
  - "Engine/**/*.cpp"
  - "Engine/**/*.h"
---
# Logging

> This file **extends** [cpp/coding-style.md](../cpp/coding-style.md) with DeltaEngine's structured logging conventions.

Use the `DLOG` macro everywhere. Never use `printf`, `std::cout`, or raw spdlog calls.

```cpp
// In the .h
DECLARE_LOG_CATEGORY(LogMeshRenderer)

// In the .cpp
DEFINE_LOG_CATEGORY(LogMeshRenderer)

DLOG(LogMeshRenderer, ELogLevel::Warning, "Mesh {} failed to load", meshName);
DLOG_IF(LogMeshRenderer, ELogLevel::Error, ptr == nullptr, "Null pointer: {}", context);
```
