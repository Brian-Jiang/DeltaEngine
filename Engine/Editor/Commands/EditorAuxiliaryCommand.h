#pragma once

#include "EditorIncludes.h"

#include "EditorCommandContext.h"

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorAuxiliaryCommand
{
public:
    virtual ~EditorAuxiliaryCommand() = default;
    virtual void Execute(EditorCommandContext& ctx) = 0;
};

DELTA_ENGINE_NS_END
