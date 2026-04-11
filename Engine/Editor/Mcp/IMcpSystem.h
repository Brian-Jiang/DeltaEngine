#pragma once

#include <string_view>

DELTA_ENGINE_NS_BEGIN

class McpRegistry;

class IMcpSystem
{
public:
    virtual ~IMcpSystem() = default;
    virtual std::string_view GetSystemName() const = 0;
    virtual void RegisterTools(McpRegistry& registry) = 0;
};

DELTA_ENGINE_NS_END
