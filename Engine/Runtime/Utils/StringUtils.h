#pragma once

#include "EngineIncludes.h"

#include <filesystem>
#include <string>

DELTA_ENGINE_NS_BEGIN

class StringUtils
{
public:
    DELTAENGINE_API static std::string WStringToUtf8(const std::wstring& wstr);
    DELTAENGINE_API static std::wstring Utf8ToWString(const std::string& str);

    /** Build a path from a UTF-8 string. */
    DELTAENGINE_API static std::filesystem::path Utf8ToPath(const std::string& utf8);
    /** Render a path back to a UTF-8 string. */
    DELTAENGINE_API static std::string PathToUtf8(const std::filesystem::path& p);
};

DELTA_ENGINE_NS_END
