#pragma once

#include "EngineIncludes.h"

#include <string>
#include <windows.h>

DELTA_ENGINE_NS_BEGIN

class StringUtils
{
public:
    static std::string WStringToUtf8(const std::wstring& wstr)
    {
        if (wstr.empty())
            return {};
        int size = WideCharToMultiByte(
            CP_UTF8, 0,
            wstr.data(), (int)wstr.size(),
            nullptr, 0,
            nullptr, nullptr);
        std::string result(size, 0);
        WideCharToMultiByte(
            CP_UTF8, 0,
            wstr.data(), (int)wstr.size(),
            result.data(), size,
            nullptr, nullptr);
        return result;
    }

    static std::wstring Utf8ToWString(const std::string& str)
    {
        if (str.empty())
            return {};
        int size = MultiByteToWideChar(
            CP_UTF8, 0,
            str.data(), (int)str.size(),
            nullptr, 0);
        std::wstring result(size, 0);
        MultiByteToWideChar(
            CP_UTF8, 0,
            str.data(), (int)str.size(),
            result.data(), size);
        return result;
    }
};

DELTA_ENGINE_NS_END
