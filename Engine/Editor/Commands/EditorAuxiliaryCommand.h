#pragma once

#include "EditorCommandContext.h"

DELTA_ENGINE_NS_BEGIN

class EditorAuxiliaryCommand
{
public:
    virtual ~EditorAuxiliaryCommand() = default;
    virtual void Execute(EditorCommandContext& ctx) = 0;
};

DELTA_ENGINE_NS_END
