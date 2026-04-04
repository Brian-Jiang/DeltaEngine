#pragma once

#include "EditorIncludes.h"

#include "EditorAuxiliaryCommand.h"

#include <filesystem>

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorAuxiliaryCommand_SaveScene : public EditorAuxiliaryCommand
{
public:
    void Execute(EditorCommandContext& ctx) override;
};

class DELTAEDITOR_API EditorAuxiliaryCommand_LoadScene : public EditorAuxiliaryCommand
{
public:
    explicit EditorAuxiliaryCommand_LoadScene(std::filesystem::path scenePath);
    void Execute(EditorCommandContext& ctx) override;

private:
    std::filesystem::path m_scenePath;
};

DELTA_ENGINE_NS_END
