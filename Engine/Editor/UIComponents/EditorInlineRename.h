#pragma once

#include "EngineIncludes.h"

DELTA_ENGINE_NS_BEGIN

class DELTAEDITOR_API EditorInlineRename
{
public:
    enum class Result
    {
        None,
        Committed,
        Cancelled
    };

    void Begin(const std::string& initialName);
    void Clear();
    bool IsActive() const { return m_active; }

    const char* GetBuffer() const { return m_buf; }

    Result Draw();

private:
    bool  m_active       = false;
    bool  m_requestFocus = false;
    char  m_buf[512]     = {};
};

DELTA_ENGINE_NS_END
