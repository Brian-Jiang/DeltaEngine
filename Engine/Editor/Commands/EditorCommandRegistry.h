#pragma once

#include "EditorCommand.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

DELTA_ENGINE_NS_BEGIN

using CommandFactory = std::function<std::unique_ptr<EditorCommand>()>;

class EditorCommandRegistry
{
public:
    DELTAEDITOR_API static EditorCommandRegistry& Get();

    DELTAEDITOR_API void Register(std::string_view typeName, CommandFactory factory);
    DELTAEDITOR_API std::unique_ptr<EditorCommand> Create(std::string_view typeName) const;

private:
    std::unordered_map<std::string, CommandFactory> m_factories;
};

template<typename T>
struct CommandRegistrar
{
    CommandRegistrar()
    {
        EditorCommandRegistry::Get().Register(
            T::StaticTypeName(),
            [] { return std::make_unique<T>(); }
        );
    }
};

DELTA_ENGINE_NS_END
