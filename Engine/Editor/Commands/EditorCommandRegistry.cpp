#include "EditorCommandRegistry.h"

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
        DLOG(LogEditorCommand, ELogLevel::Error, "[Command Registry] Unknown command type: {}", typeName);
        return nullptr;
    }
    return it->second();
}

std::vector<std::string> EditorCommandRegistry::GetCommandNames() const
{
    std::vector<std::string> names;
    names.reserve(m_factories.size());
    for (auto& [name, _] : m_factories)
        names.push_back(name);
    return names;
}
