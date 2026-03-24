#include "EditorCommandRegistry.h"

#include <cstdio>

using namespace DeltaEngine;

EditorCommandRegistry& EditorCommandRegistry::Get()
{
    static EditorCommandRegistry instance;
    return instance;
}

void EditorCommandRegistry::Register(std::string_view typeName, CommandFactory factory)
{
    m_factories[std::string(typeName)] = std::move(factory);
}

std::unique_ptr<EditorCommand> EditorCommandRegistry::Create(std::string_view typeName) const
{
    auto it = m_factories.find(std::string(typeName));
    if (it == m_factories.end())
    {
        std::printf("[EditorCommandRegistry] Unknown command type: %.*s\n",
            static_cast<int>(typeName.size()), typeName.data());
        return nullptr;
    }
    return it->second();
}
